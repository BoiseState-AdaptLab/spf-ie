#include <iostream>
#include <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/StringRef.h"

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
    void printStmtInfo() {
        SourceManager* sm = &Context->getSourceManager();
        for (std::vector<Stmt>::size_type i = 0; i != stmts.size(); ++i) {
            llvm::StringRef stmtString = clang::Lexer::getSourceText(
                clang::CharSourceRange::getTokenRange(
                    stmts[i]->getSourceRange()),
                Context->getSourceManager(), Context->getLangOpts());
            llvm ::outs() << "S" << i << ": " << stmtString << " ("
                          << stmts[i]->getStmtClassName() << ")\n";
        }
    }

   private:
    ASTContext* Context;
    std::vector<Stmt*> stmts;

    // process the body of a control structure
    void processBody(Stmt* stmt) {
        CompoundStmt* asCompoundStmt = dyn_cast<CompoundStmt>(stmt);
        if (asCompoundStmt) {
            for (auto it = asCompoundStmt->body_begin();
                 it != asCompoundStmt->body_end(); ++it) {
                processSingleStmt(*it);
            }
        } else {
            processSingleStmt(stmt);
        }
    }

    // process one statement, recursing if compound structure
    void processSingleStmt(Stmt* stmt) {
        ForStmt* asForStmt = dyn_cast<ForStmt>(stmt);
        if (asForStmt) {
            processBody(asForStmt->getBody());
            return;
        }
        IfStmt* asIfStmt = dyn_cast<IfStmt>(stmt);
        if (asIfStmt) {
            processBody(asIfStmt->getThen());
            return;
        }
        addStmt(stmt);
    }

    // add info about a stmt to the Builder
    void addStmt(Stmt* stmt) { stmts.push_back(stmt); }
};
}  // namespace pdfg_c

