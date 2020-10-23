#ifndef SPFIE_UTILS_HPP
#define SPFIE_UTILS_HPP

#include <map>
#include <string>

#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"

using namespace clang;

namespace spf_ie {

/*!
 * \class Utils
 *
 * \brief A helper class with various methods for string conversion.
 */
class Utils {
   public:
    //! Print an error to standard error and exit with error status
    static void printErrorAndExit(std::string message);

    //! Print an error to standard error, including the context of a
    //! statement in the source code, and exit with error status
    static void printErrorAndExit(std::string message, clang::Stmt* stmt);

    //! Print a line (horizontal separator) to standard output
    static void printSmallLine();

    //! Get the source code of a statement as a string
    static std::string stmtToString(clang::Stmt* stmt);

    //! Get a copy of the given string with all instances of the substring to
    //! find replaced as specified
    //! \param[in] input String to perform substitutions on (will not be
    //! modified)
    //! \param[in] toFind String to find (and replace) within input
    //! \param[in] replaceWith String to use as replacement
    static std::string replaceInString(std::string input, std::string toFind,
                                       std::string replaceWith);

    //! Retrieve "all" array accesses, from left to right, contained in an
    //! expression.
    //! Recurses into BinaryOperators.
    //! \param[in] expr Expression to process
    //! \param[out] currentList List of accesses
    static void getExprArrayAccesses(
        Expr* expr, std::vector<ArraySubscriptExpr*>& currentList);

    //! Get a string representation of a binary operator
    static std::string binaryOperatorKindToString(BinaryOperatorKind bo);

   private:
    //! String representations of valid operators for use in constraints
    static const std::map<BinaryOperatorKind, std::string> operatorStrings;

    Utils() = delete;
};

}  // namespace spf_ie

#endif
