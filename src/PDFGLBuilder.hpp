#include <iostream>
#include <stack>
#include <vector>

#include "IterSpace.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/StringRef.h"
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
    void printStmtInfo() {
        SourceManager* sm = &Context->getSourceManager();
        llvm::outs() << "Statements:\n";
        for (std::vector<Stmt>::size_type i = 0; i != stmts.size(); ++i) {
            llvm::outs() << "S" << i << ": " << stmtToString(stmts[i]) << "\n";
        }
        llvm::outs() << "\nIteration spaces:\n";
        for (std::vector<IterSpace>::size_type i = 0; i != iterSpaces.size();
             ++i) {
            llvm::outs() << "S" << i << ": "
                         << iterSpaceToString(&iterSpaces[i]) << "\n";
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

    void printErrorAndExit(llvm::StringRef message) {
        llvm::errs() << "ERROR: " << message << "\n";
        exit(1);
    }

    // various utility functions

    llvm::StringRef stmtToString(Stmt* stmt) {
        return clang::Lexer::getSourceText(
            clang::CharSourceRange::getTokenRange(stmt->getSourceRange()),
            Context->getSourceManager(), Context->getLangOpts());
    }

    llvm::StringRef exprToString(Expr* expr) {
        return clang::Lexer::getSourceText(
            clang::CharSourceRange::getTokenRange(expr->getSourceRange()),
            Context->getSourceManager(), Context->getLangOpts());
    }

    std::string iterSpaceToString(IterSpace* iterSpace) {
        std::string output;
        llvm::raw_string_ostream os(output);
        if (!iterSpace->constraints.empty()) {
            os << "{[";
            for (auto it = iterSpace->iterators.begin();
                 it != iterSpace->iterators.end(); ++it) {
                if (it != iterSpace->iterators.begin()) {
                    os << ",";
                }
                os << (*it)->getNameAsString();
            }
            os << "]: ";
            for (auto it = iterSpace->constraints.begin();
                 it != iterSpace->constraints.end(); ++it) {
                if (it != iterSpace->constraints.begin()) {
                    os << " and ";
                }
                BinaryOperatorKind bo = std::get<2>(**it);
                if (!operatorStrings.count(bo)) {
                    printErrorAndExit("Invalid operator type encountered.");
                }
                llvm::StringRef opString = operatorStrings.at(bo);
                os << exprToString(std::get<0>(**it)) << " " << opString << " "
                   << exprToString(std::get<1>(**it));
            }
            os << "}";
        } else {
            os << "{[]}";
        }
        return os.str();
    }

    std::map<BinaryOperatorKind, llvm::StringRef> operatorStrings = {
        {BinaryOperatorKind::BO_LT, "<"},
        {BinaryOperatorKind::BO_LE, "<="},
        {BinaryOperatorKind::BO_GT, ">"},
        {BinaryOperatorKind::BO_GE, ">="},
        {BinaryOperatorKind::BO_EQ, "=="}};
};
}  // namespace pdfg_c

