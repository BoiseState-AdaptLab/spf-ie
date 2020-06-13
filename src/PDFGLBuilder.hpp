#include <algorithm>
#include <vector>

#include "StmtInfoSet.hpp"
#include "Utils.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace pdfg_c {

class PDFGLBuilder {
   public:
    explicit PDFGLBuilder(ASTContext* Context)
        : Context(Context), largestScheduleDimension(0){};
    // entry point for each function; gather information about its statements
    void processFunction(FunctionDecl* funcDecl) {
        CompoundStmt* funcBody = cast<CompoundStmt>(funcDecl->getBody());
        processBody(funcBody);
        for (auto it = stmtInfoSets.begin(); it != stmtInfoSets.end(); ++it) {
            (*it).zeroPadScheduleDimension(largestScheduleDimension);
        }
    }

    // print collected information to stdout
    void printInfo() {
        llvm::outs() << "Statements:\n";
        Utils::printSmallLine();
        for (std::vector<Stmt>::size_type i = 0; i != stmts.size(); ++i) {
            llvm::outs() << "S" << i << ": "
                         << Utils::stmtToString(stmts[i], Context) << "\n";
        }
        llvm::outs() << "\nIteration spaces:\n";
        Utils::printSmallLine();
        for (std::vector<StmtInfoSet>::size_type i = 0;
             i != stmtInfoSets.size(); ++i) {
            llvm::outs() << "S" << i << ": "
                         << stmtInfoSets[i].getIterSpaceString(Context) << "\n";
        }
        llvm::outs() << "\nExecution schedules:\n";
        Utils::printSmallLine();
        for (std::vector<StmtInfoSet>::size_type i = 0;
             i != stmtInfoSets.size(); ++i) {
            llvm::outs() << "S" << i << ": "
                         << stmtInfoSets[i].getExecScheduleString(Context)
                         << "\n";
        }
    }

   private:
    ASTContext* Context;
    std::vector<Stmt*> stmts;
    std::vector<StmtInfoSet> stmtInfoSets;
    StmtInfoSet currentStmtInfoSet;
    int largestScheduleDimension;

    // process the body of a control structure
    void processBody(Stmt* stmt) {
        CompoundStmt* asCompoundStmt = dyn_cast<CompoundStmt>(stmt);
        if (asCompoundStmt) {
            for (auto it : asCompoundStmt->body()) {
                processSingleStmt(it);
            }
        } else {
            processSingleStmt(stmt);
        }
    }

    // process one statement, recursing if compound structure
    void processSingleStmt(Stmt* stmt) {
        ForStmt* asForStmt = dyn_cast<ForStmt>(stmt);
        if (asForStmt) {
            currentStmtInfoSet.advanceSchedule();
            currentStmtInfoSet.enterFor(asForStmt);
            processBody(asForStmt->getBody());
            currentStmtInfoSet.exitFor();
            return;
        }
        IfStmt* asIfStmt = dyn_cast<IfStmt>(stmt);
        if (asIfStmt) {
            currentStmtInfoSet.enterIf(asIfStmt);
            processBody(asIfStmt->getThen());
            currentStmtInfoSet.exitIf();
            return;
        }

        currentStmtInfoSet.advanceSchedule();
        addStmt(stmt);
    }

    // add info about a stmt to the PDFGLBuilder
    void addStmt(Stmt* stmt) {
        stmts.push_back(stmt);
        largestScheduleDimension =
            std::max(largestScheduleDimension,
                     currentStmtInfoSet.getScheduleDimension());
        stmtInfoSets.push_back(StmtInfoSet(&currentStmtInfoSet));
    }
};
}  // namespace pdfg_c

