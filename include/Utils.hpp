#ifndef SPFIE_UTILS_HPP
#define SPFIE_UTILS_HPP

#include <map>
#include <string>

#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"

//! Base name (will be followed by a unique string) for use in variable
//! substitutions
#define REPLACEMENT_VAR_BASE_NAME "_rVar"

using namespace clang;

namespace spf_ie {

/*!
 * \class Utils
 *
 * \brief A helper class with various methods for string conversion.
 */
class Utils {
public:
  Utils() = delete;

  //! Print an error to standard error and exit with error status
  static void printErrorAndExit(const std::string &message);

  //! Print an error to standard error, including the context of a
  //! statement in the source code, and exit with error status
  static void printErrorAndExit(const std::string &message, clang::Stmt *stmt);

  //! Get the source code of a statement as a string
  static std::string stmtToString(clang::Stmt *stmt);

  //! Get a type as string, with any arrays replaced with pointers.
  //! For example, the type int[][] would become int**.
  static std::string typeToArrayStrippedString(const clang::Type
                                               *originalType);

  //! Get a string representation of a binary operator
  static std::string binaryOperatorKindToString(BinaryOperatorKind bo);

  //! Check whether the provided expression is a variable or numeric literal (as opposed to neither of the two)
  //! \param[in] expr Expression to check
  //! \return True iff the expression is a variable or numeric literal
  static bool isVarOrNumericLiteral(const Expr *expr);

  //! Retrieve all "interesting" components that we might be concerned with,
  //! from left to right, contained in an expression.
  //! Recurses into both sides of BinaryOperators.
  //! \param[in] expr Expression to process
  //! \param[out] currentList List of accesses
  //! \param[in] includeCallExprs Whether to include function calls in collected components
  static void collectComponentsFromCompoundExpr(
      Expr *expr, std::vector<Expr *> &currentList, bool includeCallExprs = false);

  //! Get a unique variable name to use in substitutions
  static std::string getVarReplacementName();

private:
  //! String representations of valid operators for use in constraints
  static const std::map<BinaryOperatorKind, std::string> operatorStrings;

  //! Number to be used (and incremented) when creating replacement variable
  //! names
  static unsigned int replacementVarNumber;
};

}  // namespace spf_ie

#endif
