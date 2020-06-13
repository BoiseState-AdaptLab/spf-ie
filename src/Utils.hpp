#ifndef PDFG_UTILS_HPP
#define PDFG_UTILS_HPP

#include <map>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace pdfg_c {
class Utils {
   public:
    static void printErrorAndExit(llvm::StringRef message) {
        llvm::errs() << "ERROR: " << message << "\n";
        exit(1);
    }

    static llvm::StringRef stmtToString(Stmt* stmt, ASTContext* Context) {
        return Lexer::getSourceText(
            CharSourceRange::getTokenRange(stmt->getSourceRange()),
            Context->getSourceManager(), Context->getLangOpts());
    }

    static llvm::StringRef exprToString(Expr* expr, ASTContext* Context) {
        return Lexer::getSourceText(
            CharSourceRange::getTokenRange(expr->getSourceRange()),
            Context->getSourceManager(), Context->getLangOpts());
    }

    static llvm::StringRef binaryOperatorKindToString(BinaryOperatorKind bo,
                                                      ASTContext* Context) {
        if (!operatorStrings.count(bo)) {
            printErrorAndExit("Invalid operator type encountered.");
        }
        return operatorStrings.at(bo);
    }

   private:
    // valid operators for use in constraints
    static std::map<BinaryOperatorKind, llvm::StringRef> operatorStrings;

    Utils() = delete;
};

std::map<BinaryOperatorKind, llvm::StringRef> Utils::operatorStrings = {
    {BinaryOperatorKind::BO_LT, "<"},  {BinaryOperatorKind::BO_LE, "<="},
    {BinaryOperatorKind::BO_GT, ">"},  {BinaryOperatorKind::BO_GE, ">="},
    {BinaryOperatorKind::BO_EQ, "=="}, {BinaryOperatorKind::BO_NE, "!="}};

}  // namespace pdfg_c

#endif
