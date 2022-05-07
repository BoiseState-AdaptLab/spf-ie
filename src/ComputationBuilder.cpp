#include "ComputationBuilder.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Utils.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "iegenlib.h"

using namespace clang;

namespace spf_ie {

PositionContext *ComputationBuilder::positionContext;

/* ComputationBuilder */

std::map<std::string, Computation *> ComputationBuilder::subComputations;
const std::unordered_set<std::string>
    ComputationBuilder::reservedFuncNames = {"sqrt", "ceil", "floor", "pow", "abs", "log", "log10"};

ComputationBuilder::ComputationBuilder() = default;

iegenlib::Computation *
ComputationBuilder::buildComputationFromFunction(FunctionDecl *funcDecl) {
  auto *funcBody = dyn_cast<CompoundStmt>(funcDecl->getBody());
  if (!funcBody) {
    Utils::printErrorAndExit("Invalid function body", funcDecl->getBody());
  }

  positionContext = new PositionContext();
  computation = new iegenlib::Computation(funcDecl->getNameAsString());

  // add function parameters to the Computation
  for (const auto *param: funcDecl->parameters()) {
    computation->addParameter(param->getNameAsString(),
                              Utils::typeToArrayStrippedString(param->getOriginalType().getTypePtr()));
  }

  // collect function body info and add it to the Computation
  processBody(funcBody);

  // sanity check Computation completeness
  if (!computation->isComplete()) {
    Utils::printErrorAndExit(
        "Computation is in an inconsistent/incomplete state after "
        "building from function '" +
            funcDecl->getQualifiedNameAsString() +
            "'. This should not be possible and most likely indicates a bug.",
        funcBody);
  }

  return computation;
}

void ComputationBuilder::processBody(clang::Stmt *stmt) {
  if (auto *asCompoundStmt = dyn_cast<CompoundStmt>(stmt)) {
    for (auto it: asCompoundStmt->body()) {
      processSingleStmt(it);
    }
  } else {
    processSingleStmt(stmt);
  }
}

void ComputationBuilder::processSingleStmt(clang::Stmt *stmt) {
  // fail on disallowed statement types
  if (isa<WhileStmt>(stmt) || isa<CompoundStmt>(stmt) ||
      isa<SwitchStmt>(stmt) || isa<DoStmt>(stmt) || isa<LabelStmt>(stmt) ||
      isa<AttributedStmt>(stmt) || isa<GotoStmt>(stmt) ||
      isa<ContinueStmt>(stmt) || isa<BreakStmt>(stmt)) {
    Utils::printErrorAndExit("Unsupported stmt type " +
                                 std::string(stmt->getStmtClassName()),
                             stmt);
  }
  // reset statement string replacement list
  stmtSourceCodeReplacements.clear();
  this->dataAccesses = DataAccessHandler();

  if (auto *asForStmt = dyn_cast<ForStmt>(stmt)) {
    positionContext->schedule.advanceSchedule();
    positionContext->enterFor(asForStmt);
    processBody(asForStmt->getBody());
    positionContext->exitFor();
  } else if (auto *asIfStmt = dyn_cast<IfStmt>(stmt)) {
    if (asIfStmt->getConditionVariable()) {
      Utils::printErrorAndExit(
          "If statement condition variable declarations are unsupported",
          asIfStmt);
    }
    positionContext->enterIf(asIfStmt);
    processBody(asIfStmt->getThen());
    positionContext->exitIf();
    // treat else clause (if present) as another if statement, but with
    // condition inverted
    if (asIfStmt->hasElseStorage()) {
      positionContext->enterIf(asIfStmt, true);
      processBody(asIfStmt->getElse());
      positionContext->exitIf();
    }
  } else if (auto *asCallExpr = dyn_cast<CallExpr>(stmt)) {
    positionContext->schedule.advanceSchedule();
    inlineFunctionCall(asCallExpr);
  } else {
    positionContext->schedule.advanceSchedule();

    // gather data accesses
    std::map<std::string, std::string> functionCallValueReplacements;
    if (auto *asDeclStmt = dyn_cast<DeclStmt>(stmt)) {
      for (auto *decl: asDeclStmt->decls()) {
        auto *varDecl = cast<VarDecl>(decl);
        std::string varName = varDecl->getNameAsString();
        // If this declaration's variable is already registered as a data space, this is another declaration by that name.
        if (computation->isDataSpace(varName)) {
          Utils::printErrorAndExit(
              "Declaring a variable with a name that has already been used in another scope is disallowed",
              asDeclStmt);
        } else {
          this->varDecls.emplace(varName, varDecl->getType());
        }
        if (varDecl->hasInit()) {
          processComplexExpr(varDecl->getInit(), true);
          this->dataAccesses.processWriteToScalarName(varName);
        }
      }
    } else if (auto *asBinOper = dyn_cast<BinaryOperator>(stmt)) {
      if (asBinOper->isAssignmentOp()) {
        this->dataAccesses.processExprAsWrite(asBinOper->getLHS());
        if (asBinOper->isCompoundAssignmentOp()) {
          processComplexExpr(asBinOper->getLHS(), true);
        }
        processComplexExpr(asBinOper->getRHS(), true);
      } else {
        processComplexExpr(asBinOper, false);
      }
    }

    addStmt(stmt);
  }
}

void ComputationBuilder::addStmt(clang::Stmt *clangStmt) {
  // disallow statements following any return
  if (haveFoundAReturn) {
    Utils::printErrorAndExit(
        "Found a statement following a return statement. Returns are only allowed at the end of functions.",
        clangStmt);
  }
  // avoid typical Stmt handling for a return Stmt
  if (auto *asReturnStmt = dyn_cast<ReturnStmt>(clangStmt)) {
    processReturnStmt(asReturnStmt);
    return;
  }

  // build IEGenLib Stmt and add it to the Computation
  auto *newStmt = new iegenlib::Stmt();
  // source code
  std::string stmtSourceCode = Utils::stmtToString(clangStmt);
  for (const auto &replacement: stmtSourceCodeReplacements) {
    stmtSourceCode = iegenlib::replaceInString(stmtSourceCode, replacement.first, replacement.second);
  }
  // append semicolon if absent
  if (stmtSourceCode.back() != ';') {
    stmtSourceCode += ';';
  }
  newStmt->setStmtSourceCode(stmtSourceCode);
  // iteration space
  std::string iterationSpace = positionContext->getIterSpaceString();
  newStmt->setIterationSpace(iterationSpace);
  // execution schedule
  std::string executionSchedule = positionContext->getExecScheduleString();
  newStmt->setExecutionSchedule(executionSchedule);
  // data accesses
  std::vector<std::pair<std::string, std::string>> dataReads;
  std::vector<std::pair<std::string, std::string>> dataWrites;
  for (auto &it_accesses: this->dataAccesses.stmtDataAccesses) {
    std::string dataSpaceAccessed = it_accesses.name;
    // enforce loop invariance
    if (!it_accesses.isRead) {
      for (const auto &invariantGroup: positionContext->invariants) {
        if (std::find(
            invariantGroup.begin(), invariantGroup.end(),
            dataSpaceAccessed) != invariantGroup.end()) {
          Utils::printErrorAndExit(
              "Code may not modify loop-invariant data "
              "space '" +
                  dataSpaceAccessed + "'",
              clangStmt);
        }
      }
    }
    // insert data access
    if (it_accesses.isRead) {
      newStmt->addRead(dataSpaceAccessed,
                       positionContext->getDataAccessString(&it_accesses));
    } else {
      newStmt->addWrite(dataSpaceAccessed,
                        positionContext->getDataAccessString(&it_accesses));
    }
  }

  // add Computation data spaces
  auto stmtDataSpaces = this->dataAccesses.dataSpacesAccessed;
  for (const auto &dataSpaceName:
      this->dataAccesses.dataSpacesAccessed) {
    if (!computation->isDataSpace(dataSpaceName)) {
      computation->addDataSpace(dataSpaceName,
                                Utils::typeToArrayStrippedString(varDecls.at(dataSpaceName).getTypePtr()));
    }
  }

  // insert the finished statement into the Computation
  computation->addStmt(newStmt);
}

void ComputationBuilder::processReturnStmt(clang::ReturnStmt *returnStmt) {
  haveFoundAReturn = true;
  if (positionContext->nestLevel != 0) {
    Utils::printErrorAndExit("Return within nested structures is disallowed.", returnStmt);
  }

  if (auto *returnedValue = returnStmt->getRetValue()) {
    if (!Utils::isVarOrNumericLiteral(returnedValue)) {
      Utils::printErrorAndExit("Return value is too complex, must be data space or number literal.", returnedValue);
    }
    computation->addReturnValue(Utils::stmtToString(returnedValue));
  }
}

std::string ComputationBuilder::inlineFunctionCall(CallExpr *callExpr) {
  // extract call
  auto *callee = callExpr->getDirectCallee();
  if (!callee) {
    Utils::printErrorAndExit("Cannot processes this kind of call expression", callExpr);
  }
  std::string calleeName = callee->getNameAsString();

  // get arguments
  std::vector<clang::Expr *> callArgs;
  std::vector<std::string> callArgStrings;
  for (unsigned int i = 0; i < callExpr->getNumArgs(); ++i) {
    auto *arg = callExpr->getArg(i)->IgnoreParenImpCasts();
    if (!Utils::isVarOrNumericLiteral(arg)) {
      Utils::printErrorAndExit(
          "Argument passed to function is too complex (must be a data space or a numeric literal)",
          arg);
    }
    callArgs.emplace_back(arg);
    callArgStrings.emplace_back(Utils::stmtToString(arg));
  }

  // check if this is a reserved function, avoiding inlining if so
  if (reservedFuncNames.count(calleeName)) {
    // mark data space arguments as read
    for (unsigned int i = 0; i < callArgs.size(); ++i) {
      if (computation->isDataSpace(callArgStrings[i])) {
        this->dataAccesses.processExprAsRead(callArgs[i]);
      }
    }
    // stop processing here and discard return value as irrelevant
    return std::string();
  }

  // find function definition and build Computation from it if necessary
  auto *calleeDefinition = callee->getDefinition();
  if (!calleeDefinition) {
    Utils::printErrorAndExit("Cannot find definition for called function", callExpr);
  }
  if (!subComputations.count(calleeName)) {
    // build Computation from calleeDefinition, if we haven't done so already
    PositionContext oldContext = *positionContext;
    auto builder = new ComputationBuilder();
    subComputations[calleeName] = builder->buildComputationFromFunction(calleeDefinition);
    *positionContext = oldContext;
    delete builder;
  }

  auto appendResult = computation->appendComputation(subComputations[calleeName],
                                                     positionContext->getIterSpaceString(),
                                                     positionContext->getExecScheduleString(),
                                                     callArgStrings);

  // move to the next schedule position immediately after the last one used by inlined statements
  positionContext->schedule.skipToPosition(appendResult.tuplePosition);
  positionContext->schedule.advanceSchedule();

  // enforce no multiple return
  if (appendResult.returnValues.size() > 1) {
    Utils::printErrorAndExit("Function call returned multiple values", callExpr);
  }

  return (appendResult.returnValues.empty() ?
          std::string() : appendResult.returnValues.back());
}

void ComputationBuilder::processComplexExpr(Expr *expr, bool processReads) {
  std::vector<Expr *> components;
  Utils::collectComponentsFromCompoundExpr(expr, components, true);
  for (const auto &component: components) {
    if (processReads) {
      if (isa<ArraySubscriptExpr>(component)) {
        this->dataAccesses.processExprAsRead(component);
      } else if (auto *asDeclRefExpr = dyn_cast<DeclRefExpr>(component)) {
        if (!ComputationBuilder::positionContext->isIteratorName(asDeclRefExpr->getDecl()->getNameAsString())) {
          this->dataAccesses.processExprAsRead(component);
        }
      }
    }
    if (auto *asCallExpr = dyn_cast<CallExpr>(component)) {
      std::string returnValue = inlineFunctionCall(asCallExpr);
      if (!returnValue.empty()) {
        this->stmtSourceCodeReplacements.emplace(Utils::stmtToString(asCallExpr), returnValue);
        if (processReads) {
          this->dataAccesses.processReadToScalarName(returnValue);
        }
      }
    }
  }
}

}  // namespace spf_ie
