#include "Utils.hpp"

#include <map>

#include "PDFGDriver.hpp"
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

void Utils::printErrorAndExit(llvm::StringRef message) {
    printErrorAndExit(message, nullptr);
}

void Utils::printErrorAndExit(llvm::StringRef message, Stmt* stmt) {
    llvm::errs() << "ERROR: " << message << "\n";
    if (stmt) {
        llvm::errs() << "At "
                     << stmt->getBeginLoc().printToString(
                            Context->getSourceManager())
                     << ":\n"
                     << stmtToString(stmt) << "\n";
    }
    exit(1);
}

void Utils::printSmallLine() { llvm::outs() << "---------------\n"; }

std::string Utils::stmtToString(Stmt* stmt) {
    return Lexer::getSourceText(
               CharSourceRange::getTokenRange(stmt->getSourceRange()),
               Context->getSourceManager(), Context->getLangOpts())
        .str();
}

std::string Utils::exprToString(Expr* expr) {
    return Lexer::getSourceText(
               CharSourceRange::getTokenRange(expr->getSourceRange()),
               Context->getSourceManager(), Context->getLangOpts())
        .str();
}

std::string Utils::binaryOperatorKindToString(BinaryOperatorKind bo) {
    if (!operatorStrings.count(bo)) {
        printErrorAndExit("Invalid operator type encountered.");
    }
    return operatorStrings.at(bo).str();
}

std::map<BinaryOperatorKind, llvm::StringRef> Utils::operatorStrings = {
    {BinaryOperatorKind::BO_LT, "<"}, {BinaryOperatorKind::BO_LE, "<="},
    {BinaryOperatorKind::BO_GT, ">"}, {BinaryOperatorKind::BO_GE, ">="},
    {BinaryOperatorKind::BO_EQ, "="}, {BinaryOperatorKind::BO_NE, "!="}};

}  // namespace pdfg_c
