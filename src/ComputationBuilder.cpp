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

/* ComputationBuilder */

std::map<std::string, Computation *> ComputationBuilder::subComputations;

ComputationBuilder::ComputationBuilder() = default;

iegenlib::Computation *
ComputationBuilder::buildComputationFromFunction(FunctionDecl *funcDecl) {
  auto *funcBody = dyn_cast<CompoundStmt>(funcDecl->getBody());
  if (!funcBody) {
    Utils::printErrorAndExit("Invalid function body", funcDecl->getBody());
  }

  context = PositionContext();
  computation = new iegenlib::Computation(funcDecl->getNameAsString());

  // add function parameters to the Computation
  for (const auto *param: funcDecl->parameters()) {
    computation->addParameter(param->getNameAsString(), param->getOriginalType().getAsString());
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

  if (auto *asForStmt = dyn_cast<ForStmt>(stmt)) {
    context.schedule.advanceSchedule();
    context.enterFor(asForStmt);
    processBody(asForStmt->getBody());
    context.exitFor();
  } else if (auto *asIfStmt = dyn_cast<IfStmt>(stmt)) {
    if (asIfStmt->getConditionVariable()) {
      Utils::printErrorAndExit(
          "If statement condition variable declarations are unsupported",
          asIfStmt);
    }
    context.enterIf(asIfStmt);
    processBody(asIfStmt->getThen());
    context.exitIf();
    // treat else clause (if present) as another if statement, but with
    // condition inverted
    if (asIfStmt->hasElseStorage()) {
      context.enterIf(asIfStmt, true);
      processBody(asIfStmt->getElse());
      context.exitIf();
    }
  } else if (auto *asCallExpr = dyn_cast<CallExpr>(stmt)) {
    // TODO: detect function calls that are not the only thing in the statement
    auto *callee = asCallExpr->getDirectCallee();
    if (!callee) {
      Utils::printErrorAndExit("Cannot processes this kind of call expression", asCallExpr);
    }
    auto *calleeDefinition = callee->getDefinition();
    context.schedule.advanceSchedule();
    std::string calleeName = calleeDefinition->getNameAsString();
    if (!subComputations.count(calleeName)) {
      // build Computation from calleeDefinition, if we haven't done so already
      auto builder = new ComputationBuilder();
      subComputations[calleeName] = builder->buildComputationFromFunction(calleeDefinition);
    }
    std::vector<std::string> callArgStrings;
    for (unsigned int i = 0; i < asCallExpr->getNumArgs(); ++i) {
      auto *arg = asCallExpr->getArg(i)->IgnoreParenImpCasts();
      if (!Utils::isVarOrNumericLiteral(arg)) {
        Utils::printErrorAndExit(
            "Argument passed to function is too complex (must be a data space or a numeric literal)",
            arg);
      }
      callArgStrings.emplace_back(Utils::stmtToString(arg));
    }
    auto appendResult = computation->appendComputation(subComputations[calleeName],
                                                       context.getIterSpaceString(),
                                                       context.getExecScheduleString(),
                                                       callArgStrings);

    context.schedule.skipToPosition(appendResult.tuplePosition);

    // TODO: handle return value
  } else {
    context.schedule.advanceSchedule();
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

  // capture reads and writes made in statement
  DataAccessHandler dataAccesses;
  if (auto *asDeclStmt = dyn_cast<DeclStmt>(clangStmt)) {
    auto *decl = cast<VarDecl>(asDeclStmt->getSingleDecl());
    computation->addDataSpace(decl->getNameAsString(), decl->getType().getUnqualifiedType().getAsString());
    if (decl->hasInit()) {
      dataAccesses.processAsReads(decl->getInit());
    }
  } else if (auto *asBinOper = dyn_cast<BinaryOperator>(clangStmt)) {
    if (auto *lhsAsArrayAccess =
        dyn_cast<ArraySubscriptExpr>(asBinOper->getLHS())) {
      dataAccesses.processAsWrite(lhsAsArrayAccess);
    }
    if (asBinOper->isCompoundAssignmentOp()) {
      dataAccesses.processAsReads(asBinOper->getLHS());
    }
    dataAccesses.processAsReads(asBinOper->getRHS());
  }

  // build IEGenLib Stmt and add it to the Computation
  auto *newStmt = new iegenlib::Stmt();

  // source code
  std::string stmtSourceCode = Utils::stmtToString(clangStmt);
  // append semicolon if absent
  if (stmtSourceCode.back() != ';') {
    stmtSourceCode += ';';
  }
  newStmt->setStmtSourceCode(stmtSourceCode);
  // iteration space
  std::string iterationSpace = context.getIterSpaceString();
  newStmt->setIterationSpace(iterationSpace);
  // execution schedule
  std::string executionSchedule = context.getExecScheduleString();
  newStmt->setExecutionSchedule(executionSchedule);
  // data accesses
  std::vector<std::pair<std::string, std::string>> dataReads;
  std::vector<std::pair<std::string, std::string>> dataWrites;
  for (auto &it_accesses: dataAccesses.arrayAccesses) {
    std::string dataSpaceAccessed = it_accesses.arrayName;
    // enforce loop invariance
    if (!it_accesses.isRead) {
      for (const auto &invariantGroup: context.invariants) {
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
                       context.getDataAccessString(&it_accesses));
    } else {
      newStmt->addWrite(dataSpaceAccessed,
                        context.getDataAccessString(&it_accesses));
    }
  }

  computation->addStmt(newStmt);
}

void ComputationBuilder::processReturnStmt(clang::ReturnStmt *returnStmt) {
  haveFoundAReturn = true;
  if (context.nestLevel != 0) {
    Utils::printErrorAndExit("Return within nested structures is disallowed.", returnStmt);
  }

  if (auto *returnedValue = returnStmt->getRetValue()) {
    if (!Utils::isVarOrNumericLiteral(returnedValue)) {
      Utils::printErrorAndExit("Return value is too complex, must be data space or number literal.", returnedValue);
    }
    computation->addReturnValue(Utils::stmtToString(returnedValue));
  }
}

}  // namespace spf_ie
