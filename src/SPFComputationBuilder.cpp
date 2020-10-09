#include "SPFComputationBuilder.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "SPFComputation.hpp"
#include "Utils.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "iegenlib.h"

using namespace clang;

namespace spf_ie {

/* SPFComputationBuilder */

SPFComputationBuilder::SPFComputationBuilder(){};

std::unique_ptr<libspf::SPFComputation>
SPFComputationBuilder::buildComputationFromFunction(FunctionDecl* funcDecl) {
    if (CompoundStmt* funcBody = dyn_cast<CompoundStmt>(funcDecl->getBody())) {
        // reset builder components
        stmtNumber = 0;
        largestScheduleDimension = 0;
        currentStmtContext = StmtContext();
        stmtContexts.clear();
        computation = std::make_unique<libspf::SPFComputation>();

        // perform processing
        processBody(funcBody);

        // collect results into Computation
        unsigned int i = 0;
        for (auto& stmtContext : stmtContexts) {
            // iteration space
            std::string iterationSpace = stmtContext.getIterSpaceString();
            // execution schedule
            // zero-pad schedule to maximum dimension encountered
            stmtContext.schedule.zeroPadDimension(largestScheduleDimension);
            std::string executionSchedule = stmtContext.getExecScheduleString();
            // data accesses
            std::vector<std::pair<std::string, std::string>> dataReads;
            std::vector<std::pair<std::string, std::string>> dataWrites;
            for (auto& it_accesses : stmtContext.dataAccesses.arrayAccesses) {
                std::string dataSpaceAccessed =
                    Utils::stmtToString(it_accesses.second.base);
                if (!it_accesses.second.isRead) {
                    for (const auto& invariantGroup : stmtContext.invariants) {
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
                (it_accesses.second.isRead ? dataReads : dataWrites)
                    .push_back(std::make_pair(
                        dataSpaceAccessed,
                        stmtContext.getDataAccessString(&it_accesses.second)));
            }

            // insert Computation data spaces
            auto stmtDataSpaces = stmtContext.dataAccesses.dataSpaces;
            computation->dataSpaces.insert(stmtDataSpaces.begin(),
                                           stmtDataSpaces.end());

            // create and insert IEGenStmtInfo
            computation->stmtsInfoMap.emplace(
                "S" + std::to_string(i),
                libspf::IEGenStmtInfo(Utils::stmtToString(stmtContext.stmt),
                                      iterationSpace, executionSchedule,
                                      dataReads, dataWrites));

            i++;
        }
        return std::move(computation);
    } else {
        Utils::printErrorAndExit("Invalid function body", funcDecl->getBody());
    }
}

void SPFComputationBuilder::processBody(Stmt* stmt) {
    if (CompoundStmt* asCompoundStmt = dyn_cast<CompoundStmt>(stmt)) {
        for (auto it : asCompoundStmt->body()) {
            processSingleStmt(it);
        }
    } else {
        processSingleStmt(stmt);
    }
}

void SPFComputationBuilder::processSingleStmt(Stmt* stmt) {
    // fail on disallowed statement types
    if (isa<WhileStmt>(stmt) || isa<CompoundStmt>(stmt) ||
        isa<SwitchStmt>(stmt) || isa<DoStmt>(stmt) || isa<LabelStmt>(stmt) ||
        isa<AttributedStmt>(stmt) || isa<GotoStmt>(stmt) ||
        isa<ContinueStmt>(stmt) || isa<BreakStmt>(stmt) ||
        isa<CallExpr>(stmt)) {
        Utils::printErrorAndExit("Unsupported compound stmt type " +
                                     std::string(stmt->getStmtClassName()),
                                 stmt);
    }

    if (ForStmt* asForStmt = dyn_cast<ForStmt>(stmt)) {
        currentStmtContext.schedule.advanceSchedule();
        currentStmtContext.enterFor(asForStmt);
        processBody(asForStmt->getBody());
        currentStmtContext.exitFor();
    } else if (IfStmt* asIfStmt = dyn_cast<IfStmt>(stmt)) {
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

void SPFComputationBuilder::addStmt(Stmt* stmt) {
    // capture reads and writes made in statement
    if (DeclStmt* asDeclStmt = dyn_cast<DeclStmt>(stmt)) {
        VarDecl* decl = cast<VarDecl>(asDeclStmt->getSingleDecl());
        if (decl->hasInit()) {
            currentStmtContext.dataAccesses.processAsReads(decl->getInit());
        }
    } else if (BinaryOperator* asBinOper = dyn_cast<BinaryOperator>(stmt)) {
        if (ArraySubscriptExpr* lhsAsArrayAccess =
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
