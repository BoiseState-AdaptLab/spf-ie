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

SPFComputationBuilder::SPFComputationBuilder() = default;

iegenlib::Computation *
SPFComputationBuilder::buildComputationFromFunction(FunctionDecl *funcDecl) {
  if (auto *funcBody = dyn_cast<CompoundStmt>(funcDecl->getBody())) {
    // reset builder components
    stmtNumber = 0;
    largestScheduleDimension = 0;
    currentStmtContext = StmtContext();
    stmtContexts.clear();
    computation = new iegenlib::Computation();

    // add function parameters to the Computation
    for (const auto *param: funcDecl->parameters()) {
      computation->addParameter(param->getNameAsString(), param->getOriginalType().getAsString());
    }

    // collect function body info and add it to the Computation
    processBody(funcBody);
    for (auto &stmtContext: stmtContexts) {
      auto *stmt = new iegenlib::Stmt();

      // source code
      std::string stmtSourceCode = Utils::stmtToString(stmtContext.stmt);
      // append semicolon if absent
      if (stmtSourceCode.back() != ';') {
        stmtSourceCode += ';';
      }
      stmt->setStmtSourceCode(stmtSourceCode);
      // iteration space
      std::string iterationSpace = stmtContext.getIterSpaceString();
      stmt->setIterationSpace(iterationSpace);
      // execution schedule
      // zero-pad schedule to maximum dimension encountered
      stmtContext.schedule.zeroPadDimension(largestScheduleDimension);
      std::string executionSchedule = stmtContext.getExecScheduleString();
      stmt->setExecutionSchedule(executionSchedule);
      // data accesses
      std::vector<std::pair<std::string, std::string>> dataReads;
      std::vector<std::pair<std::string, std::string>> dataWrites;
      for (auto &it_accesses: stmtContext.dataAccesses.arrayAccesses) {
        std::string dataSpaceAccessed = it_accesses.arrayName;
        // enforce loop invariance
        if (!it_accesses.isRead) {
          for (const auto &invariantGroup: stmtContext.invariants) {
            if (std::find(
                invariantGroup.begin(), invariantGroup.end(),
                dataSpaceAccessed) != invariantGroup.end()) {
              Utils::printErrorAndExit(
                  "Code may not modify loop-invariant data "
                  "space '" +
                      dataSpaceAccessed + "'",
                  stmtContext.stmt);
            }
          }
        }
        // insert data access
        if (it_accesses.isRead) {
          stmt->addRead(dataSpaceAccessed,
                        stmtContext.getDataAccessString(&it_accesses));
        } else {
          stmt->addWrite(dataSpaceAccessed,
                         stmtContext.getDataAccessString(&it_accesses));
        }
      }

      // add Computation data spaces
      auto stmtDataSpaces = stmtContext.dataAccesses.dataSpaces;
      for (const auto &dataSpaceName:
          stmtContext.dataAccesses.dataSpaces) {
        computation->addDataSpace(dataSpaceName, "placeholderType");
      }

      computation->addStmt(stmt);
    }

    // sanity check Computation completeness
    if (!computation->isComplete()) {
      Utils::printErrorAndExit(
          "Computation is in an inconsistent/incomplete state after "
          "building from function '" +
              funcDecl->getQualifiedNameAsString() +
              "'. This "
              "should not be possible and most likely indicates a bug.",
          funcBody);
    }

    return computation;
  } else {
    Utils::printErrorAndExit("Invalid function body", funcDecl->getBody());
  }
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
      isa<ContinueStmt>(stmt) || isa<BreakStmt>(stmt) ||
      isa<CallExpr>(stmt)) {
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
  } else {
    currentStmtContext.schedule.advanceSchedule();
    addStmt(stmt);
  }
}

void SPFComputationBuilder::addStmt(clang::Stmt *stmt) {
  // capture reads and writes made in statement
  if (auto *asDeclStmt = dyn_cast<DeclStmt>(stmt)) {
    auto *decl = cast<VarDecl>(asDeclStmt->getSingleDecl());
    if (decl->hasInit()) {
      currentStmtContext.dataAccesses.processAsReads(decl->getInit());
    }
  } else if (auto *asBinOper = dyn_cast<BinaryOperator>(stmt)) {
    if (auto *lhsAsArrayAccess =
        dyn_cast<ArraySubscriptExpr>(asBinOper->getLHS())) {
      currentStmtContext.dataAccesses.processAsWrite(lhsAsArrayAccess);
    }
    if (asBinOper->isCompoundAssignmentOp()) {
      currentStmtContext.dataAccesses.processAsReads(asBinOper->getLHS());
    }
    currentStmtContext.dataAccesses.processAsReads(asBinOper->getRHS());
  }

  // increase largest schedule dimension, if necessary
  largestScheduleDimension = std::max(
      largestScheduleDimension, currentStmtContext.schedule.getDimension());

  // store processed statement
  currentStmtContext.stmt = stmt;
  stmtContexts.push_back(currentStmtContext);
  currentStmtContext = StmtContext(&currentStmtContext);
  stmtNumber++;
}

}  // namespace spf_ie
