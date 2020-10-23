#include "Utils.hpp"

#include <map>
#include <string>

#include "Driver.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

using namespace clang;

namespace spf_ie {

void Utils::printErrorAndExit(std::string message) {
    printErrorAndExit(message, nullptr);
}

void Utils::printErrorAndExit(std::string message, clang::Stmt* stmt) {
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

std::string Utils::stmtToString(clang::Stmt* stmt) {
    return Lexer::getSourceText(
               CharSourceRange::getTokenRange(stmt->getSourceRange()),
               Context->getSourceManager(), Context->getLangOpts())
        .str();
}

std::string Utils::replaceInString(std::string input, std::string toFind,
                                   std::string replaceWith) {
    size_t pos = input.find(toFind);
    if (pos == std::string::npos) {
        return input;
    } else {
        return replaceInString(input.replace(pos, toFind.length(), replaceWith),
                               toFind, replaceWith);
    }
}

void Utils::getExprArrayAccesses(
    Expr* expr, std::vector<ArraySubscriptExpr*>& currentList) {
    Expr* usableExpr = expr->IgnoreParenImpCasts();
    if (BinaryOperator* binOper = dyn_cast<BinaryOperator>(usableExpr)) {
        getExprArrayAccesses(binOper->getLHS(), currentList);
        getExprArrayAccesses(binOper->getRHS(), currentList);
    } else if (ArraySubscriptExpr* asArrayAccessExpr =
                   dyn_cast<ArraySubscriptExpr>(usableExpr)) {
        currentList.push_back(asArrayAccessExpr);
    }
}

std::string Utils::binaryOperatorKindToString(BinaryOperatorKind bo) {
    if (!operatorStrings.count(bo)) {
        printErrorAndExit("Invalid operator type encountered.");
    }
    return operatorStrings.at(bo);
}

const std::map<BinaryOperatorKind, std::string> Utils::operatorStrings = {
    {BinaryOperatorKind::BO_LT, "<"}, {BinaryOperatorKind::BO_LE, "<="},
    {BinaryOperatorKind::BO_GT, ">"}, {BinaryOperatorKind::BO_GE, ">="},
    {BinaryOperatorKind::BO_EQ, "="}, {BinaryOperatorKind::BO_NE, "!="}};

}  // namespace spf_ie
