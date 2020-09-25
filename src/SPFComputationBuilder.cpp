#include "SPFComputationBuilder.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include "SPFComputation.hpp"
#include "Utils.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "iegenlib.h"

using namespace clang;

namespace spf_ie {

/* SPFComputationBuilder */

SPFComputationBuilder::SPFComputationBuilder()
    : stmtNumber(0), largestScheduleDimension(0){};

SPFComputation SPFComputationBuilder::buildComputationFromFunction(
    FunctionDecl* funcDecl) {
    computation = SPFComputation();
    if (CompoundStmt* funcBody = dyn_cast<CompoundStmt>(funcDecl->getBody())) {
        processBody(funcBody);

        unsigned int i = 0;
        for (auto it = computation.stmtsInfoMap.begin();
             it != computation.stmtsInfoMap.end(); ++it) {
            it->second.iterationSpace =
                iegenlib::Set(stmtContexts[i].getIterSpaceString());
            stmtContexts[i].schedule.zeroPadDimension(largestScheduleDimension);
            it->second.executionSchedule =
                iegenlib::Relation(stmtContexts[i].getExecScheduleString());
            std::vector<std::string> readStrings =
                stmtContexts[i].getReadsStrings();
            for (auto it_reads = readStrings.begin();
                 it_reads != readStrings.end(); ++it_reads) {
                it->second.dataReads.push_back(iegenlib::Relation(*it_reads));
            }
            std::vector<std::string> writeStrings =
                stmtContexts[i].getWritesStrings();
            for (auto it_writes = writeStrings.begin();
                 it_writes != writeStrings.end(); ++it_writes) {
                it->second.dataWrites.push_back(iegenlib::Relation(*it_writes));
            }
            // TODO: add data spaces
            i++;
        }
        return computation;
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
        std::ostringstream errorMsg;
        errorMsg << "Unsupported compound stmt type "
                 << stmt->getStmtClassName();
        Utils::printErrorAndExit(errorMsg.str(), stmt);
    }

    if (ForStmt* asForStmt = dyn_cast<ForStmt>(stmt)) {
        currentStmtContext.schedule.advanceSchedule();
        currentStmtContext.enterFor(asForStmt);
        processBody(asForStmt->getBody());
        currentStmtContext.exitFor();
    } else if (IfStmt* asIfStmt = dyn_cast<IfStmt>(stmt)) {
        currentStmtContext.enterIf(asIfStmt);
        processBody(asIfStmt->getThen());
        currentStmtContext.exitIf();
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
    computation.stmtsInfoMap.emplace("S" + std::to_string(stmtNumber++),
                                     IEGenStmtInfo(Utils::stmtToString(stmt)));
    stmtContexts.push_back(currentStmtContext);
    currentStmtContext = StmtContext(&currentStmtContext);
}

}  // namespace spf_ie
