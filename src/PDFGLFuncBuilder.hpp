#include <algorithm>
#include <sstream>
#include <vector>

#include "PDFGDriver.hpp"
#include "StmtInfoSet.hpp"
#include "Utils.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace pdfg_c {

class PDFGLFuncBuilder {
   public:
    explicit PDFGLFuncBuilder() : largestScheduleDimension(0){};
    // entry point for each function; gather information about its statements
    void processFunction(FunctionDecl* funcDecl) {
        functionName = funcDecl->getQualifiedNameAsString();
        if (CompoundStmt* funcBody =
                dyn_cast<CompoundStmt>(funcDecl->getBody())) {
            processBody(funcBody);
            for (auto it = stmtInfoSets.begin(); it != stmtInfoSets.end();
                 ++it) {
                (*it).zeroPadScheduleDimension(largestScheduleDimension);
            }
        } else {
            Utils::printErrorAndExit("Invalid function body",
                                     funcDecl->getBody());
        }
    }

    // print collected information to stdout
    void printInfo() {
        llvm::outs() << "FUNCTION: " << functionName << "\n";
        Utils::printSmallLine();
        llvm::outs() << "\n";
        llvm::outs() << "Statements:\n";
        for (std::vector<Stmt>::size_type i = 0; i != stmts.size(); ++i) {
            llvm::outs() << "S" << i << ": " << Utils::stmtToString(stmts[i])
                         << "\n";
        }
        llvm::outs() << "\nIteration spaces:\n";
        for (std::vector<StmtInfoSet>::size_type i = 0;
             i != stmtInfoSets.size(); ++i) {
            llvm::outs() << "S" << i << ": "
                         << stmtInfoSets[i].getIterSpaceString() << "\n";
        }
        llvm::outs() << "\nExecution schedules:\n";
        for (std::vector<StmtInfoSet>::size_type i = 0;
             i != stmtInfoSets.size(); ++i) {
            llvm::outs() << "S" << i << ": "
                         << stmtInfoSets[i].getExecScheduleString() << "\n";
        }
        llvm::outs() << "\n";
    }

   private:
    std::string functionName;
    std::vector<Stmt*> stmts;
    std::vector<StmtInfoSet> stmtInfoSets;
    StmtInfoSet currentStmtInfoSet;
    int largestScheduleDimension;

    // process the body of a control structure
    void processBody(Stmt* stmt) {
        if (CompoundStmt* asCompoundStmt = dyn_cast<CompoundStmt>(stmt)) {
            for (auto it : asCompoundStmt->body()) {
                processSingleStmt(it);
            }
        } else {
            processSingleStmt(stmt);
        }
    }

    // process one statement, recursing if compound structure
    void processSingleStmt(Stmt* stmt) {
        // fail on disallowed statement types
        if (isa<WhileStmt>(stmt) || isa<CompoundStmt>(stmt) ||
            isa<SwitchStmt>(stmt) || isa<DoStmt>(stmt) ||
            isa<LabelStmt>(stmt) || isa<AttributedStmt>(stmt) ||
            isa<GotoStmt>(stmt) || isa<ContinueStmt>(stmt) ||
            isa<BreakStmt>(stmt)) {
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
            currentStmtInfoSet.advanceSchedule();
            addStmt(stmt);
        }
    }

    // add info about a stmt to the builder
    void addStmt(Stmt* stmt) {
        stmts.push_back(stmt);
        largestScheduleDimension =
            std::max(largestScheduleDimension,
                     currentStmtInfoSet.getScheduleDimension());
        stmtInfoSets.push_back(StmtInfoSet(&currentStmtInfoSet));
    }
};
}  // namespace pdfg_c

