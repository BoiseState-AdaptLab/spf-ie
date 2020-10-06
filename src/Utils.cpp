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

void Utils::printErrorAndExit(std::string message, Stmt* stmt) {
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
    return operatorStrings.at(bo);
}

BinaryOperatorKind Utils::invertRelationalOperator(BinaryOperatorKind bo) {
    switch (bo) {
        case BinaryOperatorKind::BO_LT:
            return BinaryOperatorKind::BO_GE;
            break;
        case BinaryOperatorKind::BO_LE:
            return BinaryOperatorKind::BO_GT;
            break;
        case BinaryOperatorKind::BO_GT:
            return BinaryOperatorKind::BO_LE;
            break;
        case BinaryOperatorKind::BO_GE:
            return BinaryOperatorKind::BO_LT;
            break;
        case BinaryOperatorKind::BO_EQ:
            return BinaryOperatorKind::BO_NE;
            break;
        case BinaryOperatorKind::BO_NE:
            return BinaryOperatorKind::BO_EQ;
            break;
        default:
            printErrorAndExit("Invalid relational operator encountered.");
    }
}

std::map<BinaryOperatorKind, std::string> Utils::operatorStrings = {
    {BinaryOperatorKind::BO_LT, "<"}, {BinaryOperatorKind::BO_LE, "<="},
    {BinaryOperatorKind::BO_GT, ">"}, {BinaryOperatorKind::BO_GE, ">="},
    {BinaryOperatorKind::BO_EQ, "="}, {BinaryOperatorKind::BO_NE, "!="}};

}  // namespace spf_ie
