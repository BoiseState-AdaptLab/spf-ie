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

void Utils::printErrorAndExit(std::string message, clang::Stmt *stmt) {
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

const std::map<BinaryOperatorKind, std::string> Utils::operatorStrings = {
    {BinaryOperatorKind::BO_LT, "<"}, {BinaryOperatorKind::BO_LE, "<="},
    {BinaryOperatorKind::BO_GT, ">"}, {BinaryOperatorKind::BO_GE, ">="},
    {BinaryOperatorKind::BO_EQ, "="}, {BinaryOperatorKind::BO_NE, "!="}};

unsigned int Utils::replacementVarNumber = 0;

}  // namespace spf_ie
