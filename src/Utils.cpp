#include "Utils.hpp"

#include <map>
#include <string>

#include "Driver.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

using namespace clang;

namespace spf_ie {

void Utils::printErrorAndExit(const std::string &message) {
  printErrorAndExit(message, nullptr);
}

void Utils::printErrorAndExit(const std::string &message, clang::Stmt *stmt) {
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

std::string Utils::stmtToString(clang::Stmt *stmt) {
  return Lexer::getSourceText(
      CharSourceRange::getTokenRange(stmt->getSourceRange()),
      Context->getSourceManager(), Context->getLangOpts())
      .str();
}

std::string Utils::typeToArrayStrippedString(const clang::Type *originalType) {
  if (originalType->isArrayType()) {
    return typeToArrayStrippedString(originalType->getArrayElementTypeNoTypeQual()) + "*";
  } else {
    return QualType(originalType, 0).getAsString();
  }
}

std::string Utils::getVarReplacementName() {
  return REPLACEMENT_VAR_BASE_NAME + std::to_string(replacementVarNumber++);
}

std::string Utils::binaryOperatorKindToString(BinaryOperatorKind bo) {
  if (!operatorStrings.count(bo)) {
    printErrorAndExit("Invalid operator type encountered.");
  }
  return operatorStrings.at(bo);
}

bool Utils::isVarOrNumericLiteral(const Expr *expr) {
  auto *plainExpr = expr->IgnoreParenImpCasts();
  return (isa<DeclRefExpr>(plainExpr)
      || isa<IntegerLiteral>(plainExpr)
      || isa<FixedPointLiteral>(plainExpr)
      || isa<FloatingLiteral>(plainExpr)
  );
}

void Utils::collectComponentsFromCompoundExpr(
    Expr *expr, std::vector<Expr *> &currentList, bool includeCallExprs) {
  Expr *usableExpr = expr->IgnoreParenImpCasts();
  if (auto *binOper = dyn_cast<BinaryOperator>(usableExpr)) {
    collectComponentsFromCompoundExpr(binOper->getLHS(), currentList, includeCallExprs);
    collectComponentsFromCompoundExpr(binOper->getRHS(), currentList, includeCallExprs);
  } else if (isa<ArraySubscriptExpr>(usableExpr)
      || isa<DeclRefExpr>(usableExpr)
      || (includeCallExprs && isa<CallExpr>(usableExpr))) {
    currentList.push_back(usableExpr);
  } else if (!Utils::isVarOrNumericLiteral(usableExpr)) {
    Utils::printErrorAndExit("Failed to process components of complex expression", expr);
  }
}

const std::map<BinaryOperatorKind, std::string> Utils::operatorStrings = {
    {BinaryOperatorKind::BO_LT, "<"}, {BinaryOperatorKind::BO_LE, "<="},
    {BinaryOperatorKind::BO_GT, ">"}, {BinaryOperatorKind::BO_GE, ">="},
    {BinaryOperatorKind::BO_EQ, "="}, {BinaryOperatorKind::BO_NE, "!="}};

unsigned int Utils::replacementVarNumber = 0;

}  // namespace spf_ie
