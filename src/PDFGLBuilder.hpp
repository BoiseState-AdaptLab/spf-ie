#include <vector>

#include "IterSpace.hpp"
#include "Utils.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace pdfg_c {

class Builder {
   public:
    explicit Builder(ASTContext* Context) : Context(Context){};
    // entry point; gather information about all statements in a function
    void processFunction(FunctionDecl* funcDecl) {
        CompoundStmt* funcBody = cast<CompoundStmt>(funcDecl->getBody());
        processBody(funcBody);
    }

    // print collected information to stdout
    void printInfo() {
        llvm::outs() << "Statements:\n";
        for (std::vector<Stmt>::size_type i = 0; i != stmts.size(); ++i) {
            llvm::outs() << "S" << i << ": "
                         << Utils::stmtToString(stmts[i], Context) << "\n";
        }
        llvm::outs() << "\nIteration spaces:\n";
        for (std::vector<IterSpace>::size_type i = 0; i != iterSpaces.size();
             ++i) {
            llvm::outs() << "S" << i << ": " << iterSpaces[i].toString(Context)
                         << "\n";
        }
        llvm::outs() << "\nExecution schedules:\n";
        llvm::outs() << "schedules placeholder\n";
    }

   private:
    ASTContext* Context;
    std::vector<Stmt*> stmts;
    std::vector<IterSpace> iterSpaces;
    IterSpace currentIterSpace;
    /* std::vector<Schedule*> schedules; */
    /* std::stack<Schedule*> currentSchedule; */

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
            currentIterSpace.enterFor(asForStmt);
            processBody(asForStmt->getBody());
            currentIterSpace.exitFor();
            return;
        }
        IfStmt* asIfStmt = dyn_cast<IfStmt>(stmt);
        if (asIfStmt) {
            currentIterSpace.enterIf(asIfStmt);
            processBody(asIfStmt->getThen());
            currentIterSpace.exitIf();
            return;
        }

        addStmt(stmt);
    }

    // add info about a stmt to the Builder
    void addStmt(Stmt* stmt) {
        stmts.push_back(stmt);
        iterSpaces.push_back(IterSpace(&currentIterSpace));
    }
};
}  // namespace pdfg_c

