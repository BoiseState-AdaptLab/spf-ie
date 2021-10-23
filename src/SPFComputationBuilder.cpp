#include "SPFComputationBuilder.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <sstream>
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

/* SPFComputationBuilder */

std::map<std::string, Computation *> SPFComputationBuilder::subComputations;

SPFComputationBuilder::SPFComputationBuilder() = default;

iegenlib::Computation *
SPFComputationBuilder::buildComputationFromFunction(FunctionDecl *funcDecl) {
  auto *funcBody = dyn_cast<CompoundStmt>(funcDecl->getBody());
  if (!funcBody) {
    Utils::printErrorAndExit("Invalid function body", funcDecl->getBody());
  }

  currentStmtContext = StmtContext();
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

void SPFComputationBuilder::processBody(clang::Stmt *stmt) {
  if (auto *asCompoundStmt = dyn_cast<CompoundStmt>(stmt)) {
    for (auto it: asCompoundStmt->body()) {
      processSingleStmt(it);
    }
  } else {
    processSingleStmt(stmt);
  }
}

void SPFComputationBuilder::processSingleStmt(clang::Stmt *stmt) {
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
    currentStmtContext.schedule.advanceSchedule();
    currentStmtContext.enterFor(asForStmt);
    processBody(asForStmt->getBody());
    currentStmtContext.exitFor();
  } else if (auto *asIfStmt = dyn_cast<IfStmt>(stmt)) {
    if (asIfStmt->getConditionVariable()) {
      Utils::printErrorAndExit(
          "If statement condition variable declarations are unsupported",
          asIfStmt);
    }
    currentStmtContext.enterIf(asIfStmt);
    processBody(asIfStmt->getThen());
    currentStmtContext.exitIf();
    // treat else clause (if present) as another if statement, but with
    // condition inverted
    if (asIfStmt->hasElseStorage()) {
      currentStmtContext.enterIf(asIfStmt, true);
      processBody(asIfStmt->getElse());
      currentStmtContext.exitIf();
    }
  } else if (auto *asCallExpr = dyn_cast<CallExpr>(stmt)) {
    // TODO: detect function calls that are not the only thing in the statement
    auto *callee = asCallExpr->getDirectCallee();
    if (!callee) {
      Utils::printErrorAndExit("Cannot processes this kind of call expression", asCallExpr);
    }
    currentStmtContext.schedule.advanceSchedule();
    std::string calleeName = callee->getNameAsString();
    if (!subComputations.count(calleeName)) {
      // build Computation from callee, if we haven't done so already
      auto builder = new SPFComputationBuilder();
      subComputations[calleeName] = builder->buildComputationFromFunction(callee);
    }
    std::vector<std::string> callArgStrings;
    for (unsigned int i = 0; i < asCallExpr->getNumArgs(); ++i) {
      auto *arg = asCallExpr->getArg(i)->IgnoreParenImpCasts();
      if (!(isa<DeclRefExpr>(arg) || isa<IntegerLiteral>(arg) || isa<FixedPointLiteral>(arg)
          || isa<FloatingLiteral>(arg))) {
        Utils::printErrorAndExit(
            "Argument passed to function is too complex (must be a data space or a numeric literal)",
            arg);
      }
      callArgStrings.emplace_back(Utils::stmtToString(arg));
    }
    auto appendResult = computation->appendComputation(subComputations[calleeName],
                                                       currentStmtContext.getIterSpaceString(),
                                                       currentStmtContext.getExecScheduleString(),
                                                       callArgStrings);

    currentStmtContext.schedule.skipToPosition(appendResult.tuplePosition);

    // TODO: handle return value
  } else {
    currentStmtContext.schedule.advanceSchedule();
    addStmt(stmt);
  }
}

void SPFComputationBuilder::addStmt(clang::Stmt *clangStmt) {
  // capture reads and writes made in statement
  DataAccessHandler dataAccesses;
  if (auto *asDeclStmt = dyn_cast<DeclStmt>(clangStmt)) {
    auto *decl = cast<VarDecl>(asDeclStmt->getSingleDecl());
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
  std::string iterationSpace = currentStmtContext.getIterSpaceString();
  newStmt->setIterationSpace(iterationSpace);
  // execution schedule
  std::string executionSchedule = currentStmtContext.getExecScheduleString();
  newStmt->setExecutionSchedule(executionSchedule);
  // data accesses
  std::vector<std::pair<std::string, std::string>> dataReads;
  std::vector<std::pair<std::string, std::string>> dataWrites;
  for (auto &it_accesses: dataAccesses.arrayAccesses) {
    std::string dataSpaceAccessed = it_accesses.arrayName;
    // enforce loop invariance
    if (!it_accesses.isRead) {
      for (const auto &invariantGroup: currentStmtContext.invariants) {
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
                       currentStmtContext.getDataAccessString(&it_accesses));
    } else {
      newStmt->addWrite(dataSpaceAccessed,
                        currentStmtContext.getDataAccessString(&it_accesses));
    }
  }

  // add Computation data spaces
  auto stmtDataSpaces = dataAccesses.dataSpaces;
  for (const auto &dataSpaceName:
      dataAccesses.dataSpaces) {
    computation->addDataSpace(dataSpaceName, "placeholderType");
  }

  computation->addStmt(newStmt);
}

}  // namespace spf_ie
