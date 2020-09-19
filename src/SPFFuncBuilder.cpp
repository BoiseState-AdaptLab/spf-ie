#include "SPFFuncBuilder.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include "Utils.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "iegenlib.h"

using namespace clang;

namespace spf_ie {

SPFFuncBuilder::SPFFuncBuilder() : largestScheduleDimension(0){};

void SPFFuncBuilder::processFunction(FunctionDecl* funcDecl) {
    functionName = funcDecl->getQualifiedNameAsString();
    if (CompoundStmt* funcBody = dyn_cast<CompoundStmt>(funcDecl->getBody())) {
        processBody(funcBody);
        for (auto it = stmtInfoSets.begin(); it != stmtInfoSets.end(); ++it) {
            (*it).zeroPadScheduleDimension(largestScheduleDimension);
        }
    } else {
        Utils::printErrorAndExit("Invalid function body", funcDecl->getBody());
    }

    std::map<std::string, iegenlib::Set> iterSpaces;
    std::map<std::string, iegenlib::Relation> executionSchedules;
    std::vector<IEGenDataAccess> reads;
    std::vector<IEGenDataAccess> writes;
    for (std::vector<StmtInfoSet>::size_type i = 0; i != stmtInfoSets.size();
         ++i) {
        std::string stmt = "S" + std::to_string(i);
        iterSpaces.emplace(stmt,
                           iegenlib::Set(stmtInfoSets[i].getIterSpaceString()));
        executionSchedules.emplace(
            stmt, iegenlib::Relation(stmtInfoSets[i].getExecScheduleString()));
        for (auto it = stmtInfoSets[i].dataReads.begin();
             it != stmtInfoSets[i].dataReads.end(); ++it) {
            /* reads.push_back(IEGenDataAccess( */
            /*     stmt, Utils::getArrayAccessString(*it), */
            /*     Utils::stmtToString(getArrayAccessInfo(*it).top()), */
            /*     iegenlib::Relation(.....))); */
        }
    }

    llvm::outs() << "iegen iterspace sets:\n";
    for (auto it = iterSpaces.begin(); it != iterSpaces.end(); ++it) {
        llvm::outs() << "stmt " << it->first
                     << ", iterspace: " << it->second.prettyPrintString()
                     << "\n";
    }
    llvm::outs() << "\n";

    llvm::outs() << "iegen execschedule relations:\n";
    for (auto it = executionSchedules.begin(); it != executionSchedules.end();
         ++it) {
        llvm::outs() << "stmt " << it->first
                     << ", schedule: " << it->second.prettyPrintString()
                     << "\n";
    }

    llvm::outs() << "\n\n";
}

void SPFFuncBuilder::printInfo() {
    llvm::outs() << "FUNCTION: " << functionName << "\n";
    Utils::printSmallLine();
    llvm::outs() << "\n";
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

void SPFFuncBuilder::processBody(Stmt* stmt) {
    if (CompoundStmt* asCompoundStmt = dyn_cast<CompoundStmt>(stmt)) {
        for (auto it : asCompoundStmt->body()) {
            processSingleStmt(it);
        }
    } else {
        processSingleStmt(stmt);
    }
}

void SPFFuncBuilder::processSingleStmt(Stmt* stmt) {
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
        currentStmtInfoSet.advanceSchedule();
        currentStmtInfoSet.enterFor(asForStmt);
        processBody(asForStmt->getBody());
        currentStmtInfoSet.exitFor();
    } else if (IfStmt* asIfStmt = dyn_cast<IfStmt>(stmt)) {
        currentStmtInfoSet.enterIf(asIfStmt);
        processBody(asIfStmt->getThen());
        currentStmtInfoSet.exitIf();
    } else {
        // capture reads and writes made in statement
        if (DeclStmt* asDeclStmt = dyn_cast<DeclStmt>(stmt)) {
            VarDecl* decl = cast<VarDecl>(asDeclStmt->getSingleDecl());
            if (decl->hasInit()) {
                if (ArraySubscriptExpr* initAsArrayAccess =
                        dyn_cast<ArraySubscriptExpr>(decl->getInit())) {
                    currentStmtInfoSet.processReads(initAsArrayAccess);
                }
            }
        } else if (BinaryOperator* asBinOper = dyn_cast<BinaryOperator>(stmt)) {
            if (ArraySubscriptExpr* lhsAsArrayAccess =
                    dyn_cast<ArraySubscriptExpr>(asBinOper->getLHS())) {
                currentStmtInfoSet.processWrite(lhsAsArrayAccess);
            }
            if (asBinOper->isCompoundAssignmentOp()) {
                currentStmtInfoSet.processReads(asBinOper->getLHS());
            }
            currentStmtInfoSet.processReads(asBinOper->getRHS());
        }

        currentStmtInfoSet.advanceSchedule();
        addStmt(stmt);
    }
}

void SPFFuncBuilder::addStmt(Stmt* stmt) {
    stmts.push_back(stmt);
    largestScheduleDimension = std::max(
        largestScheduleDimension, currentStmtInfoSet.getScheduleDimension());
    stmtInfoSets.push_back(currentStmtInfoSet);
    currentStmtInfoSet = StmtInfoSet(&currentStmtInfoSet);
}

}  // namespace spf_ie
