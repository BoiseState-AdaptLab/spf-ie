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
    computation = SPFComputation(); FunctionDecl * funcDecl) {
    functionName = funcDecl->getQualifiedNameAsString();
    if (CompoundStmt* funcBody = dyn_cast<CompoundStmt>(funcDecl->getBody())) {
        processBody(funcBody);
        for (auto it = computation.executionSchedules.begin();
             it != computation.executionSchedules.end(); ++it) {
            (*it).zeroPadScheduleDimension(largestScheduleDimension);
        }
        return computation;
    } else {
        Utils::printErrorAndExit("Invalid function body", funcDecl->getBody());
    }
}

void SPFComputationBuilder::printInfo() {
    llvm::outs() << "Statements:\n";
    for (std::vector<Stmt>::size_type i = 0; i != stmts.size(); ++i) {
        llvm::outs() << "S" << i << ": " << Utils::stmtToString(stmts[i])
                     << "\n";
    }
    llvm::outs() << "\nIteration spaces:\n";
    for (std::vector<StmtInfoSet>::size_type i = 0; i != stmtInfoSets.size();
         ++i) {
        llvm::outs() << "S" << i << ": " << stmtInfoSets[i].getIterSpaceString()
                     << "\n";
    }
    llvm::outs() << "\nExecution schedules:\n";
    for (std::vector<StmtInfoSet>::size_type i = 0; i != stmtInfoSets.size();
         ++i) {
        llvm::outs() << "S" << i << ": "
                     << stmtInfoSets[i].getExecScheduleString() << "\n";
    }
    llvm::outs() << "\nArray reads:\n";
    for (std::vector<StmtInfoSet>::size_type i = 0; i != stmtInfoSets.size();
         ++i) {
        llvm::outs() << "S" << i << ": " << stmtInfoSets[i].getReadsString()
                     << "\n";
    }
    llvm::outs() << "\nArray writes:\n";
    for (std::vector<StmtInfoSet>::size_type i = 0; i != stmtInfoSets.size();
         ++i) {
        llvm::outs() << "S" << i << ": " << stmtInfoSets[i].getWritesString()
                     << "\n";
    }
    llvm::outs() << "\n";
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
        currentStmtContext.advanceSchedule();
        currentStmtContext.enterFor(asForStmt);
        processBody(asForStmt->getBody());
        currentStmtContext.exitFor();
    } else if (IfStmt* asIfStmt = dyn_cast<IfStmt>(stmt)) {
        currentStmtContext.enterIf(asIfStmt);
        processBody(asIfStmt->getThen());
        currentStmtContext.exitIf();
    } else {
        currentStmtContext.advanceSchedule();
        addStmt(stmt);
    }
}

void SPFComputationBuilder::addStmt(Stmt* stmt) {
    // capture reads and writes made in statement
    if (DeclStmt* asDeclStmt = dyn_cast<DeclStmt>(stmt)) {
        VarDecl* decl = cast<VarDecl>(asDeclStmt->getSingleDecl());
        if (decl->hasInit()) {
            if (ArraySubscriptExpr* initAsArrayAccess =
                    dyn_cast<ArraySubscriptExpr>(decl->getInit())) {
                currentStmtContext.processReads(initAsArrayAccess);
            }
        }
    } else if (BinaryOperator* asBinOper = dyn_cast<BinaryOperator>(stmt)) {
        if (ArraySubscriptExpr* lhsAsArrayAccess =
                dyn_cast<ArraySubscriptExpr>(asBinOper->getLHS())) {
            currentStmtContext.processWrite(lhsAsArrayAccess);
        }
        if (asBinOper->isCompoundAssignmentOp()) {
            currentStmtContext.processReads(asBinOper->getLHS());
        }
        currentStmtContext.processReads(asBinOper->getRHS());
    }

    // store processed statement and its info
    largestScheduleDimension = std::max(
        largestScheduleDimension, currentStmtContext.getScheduleDimension());
    IEGenStmtInfo stmtInfo;
    stmtInfo.stmtSourceCode = Utils::stmtToString(stmt);
    stmtInfo.iterationSpace =
        iegenlib::Set(currentStmtContext.getIterSpaceString());
    stmtInfo.executionSchedule =
        iegenlib::Relation(currentStmtContext.getExecScheduleString());
    computation.stmtsInfoMap.emplace("S" + std::to_string(stmtNumber++),
                                     stmtInfo);
}

}  // namespace spf_ie
