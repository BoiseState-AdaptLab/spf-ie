#include "Utils.hpp"

#include <map>

#include "Driver.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace spf_ie {

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

std::string Utils::binaryOperatorKindToString(BinaryOperatorKind bo) {
    if (!operatorStrings.count(bo)) {
        printErrorAndExit("Invalid operator type encountered.");
    }
    return operatorStrings.at(bo).str();
}

std::string Utils::getArrayAccessString(ArraySubscriptExpr* expr) {
    std::string output;
    llvm::raw_string_ostream os(output);
    std::stack<Expr*> info = getArrayAccessInfo(expr);
    os << Utils::stmtToString(info.top()) << "(";
    info.pop();
    bool first = true;
    while (!info.empty()) {
        if (!first) {
            os << ",";
        } else {
            first = false;
        }
        std::string indexString;
        if (ArraySubscriptExpr* asArrayAccess = dyn_cast<ArraySubscriptExpr>(
                info.top()->IgnoreParenImpCasts())) {
            indexString = getArrayAccessString(asArrayAccess);
        } else {
            indexString = Utils::stmtToString(info.top());
        }
        os << indexString;
        info.pop();
    }
    os << ")";
    return os.str();
}

std::stack<Expr*> Utils::getArrayAccessInfo(ArraySubscriptExpr* fullExpr) {
    std::stack<Expr*> info;
    if (getArrayAccessInfo(fullExpr, &info)) {
        Utils::printErrorAndExit("Array dimension exceeds maximum of " +
                                     std::to_string(MAX_ARRAY_DIM),
                                 fullExpr);
    }
    return info;
}

int Utils::getArrayAccessInfo(ArraySubscriptExpr* fullExpr,
                              std::stack<Expr*>* currentInfo) {
    if (currentInfo->size() >= MAX_ARRAY_DIM) {
        return 1;
    }
    currentInfo->push(fullExpr->getIdx());
    Expr* baseExpr = fullExpr->getBase()->IgnoreParenImpCasts();
    if (ArraySubscriptExpr* baseArrayAccess =
            dyn_cast<ArraySubscriptExpr>(baseExpr)) {
        return getArrayAccessInfo(baseArrayAccess, currentInfo);
    }
    currentInfo->push(baseExpr);
    return 0;
}

std::map<BinaryOperatorKind, llvm::StringRef> Utils::operatorStrings = {
    {BinaryOperatorKind::BO_LT, "<"}, {BinaryOperatorKind::BO_LE, "<="},
    {BinaryOperatorKind::BO_GT, ">"}, {BinaryOperatorKind::BO_GE, ">="},
    {BinaryOperatorKind::BO_EQ, "="}, {BinaryOperatorKind::BO_NE, "!="}};

}  // namespace spf_ie
