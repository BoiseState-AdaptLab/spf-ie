#ifndef SPFIE_UTILS_HPP
#define SPFIE_UTILS_HPP

#include <map>

#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/StringRef.h"

//! Maximum allowed array dimension (a safe estimate to avoid stack overflow)
#define MAX_ARRAY_DIM 50

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
    static void printErrorAndExit(llvm::StringRef message);

    //! Print an error to standard error, including the context of a
    //! statement in the source code, and exit with error status
    static void printErrorAndExit(llvm::StringRef message, Stmt* stmt);

    //! Print a line (horizontal separator) to standard output
    static void printSmallLine();

    //! Get the source code of a statement as a string
    static std::string stmtToString(Stmt* stmt);

    //! Get a string representation of a binary operator
    static std::string binaryOperatorKindToString(BinaryOperatorKind bo);

    //! Get a correctly-formatted string representing an array (data) access
    static std::string getArrayAccessString(ArraySubscriptExpr* expr);

    //! Retrieve (flatly) base + all indexes accessed in a potentially
    //! multi-dimensional array access
    //! \param[in] fullExpr array access to process
    //! \return stack containing all expressions involved in array access
    //! (base and indexes)
    static std::stack<Expr*> getArrayAccessInfo(ArraySubscriptExpr* fullExpr);

    //! Do the actual recursive work of getting array access info
    //! \param[in] fullExpr array access to process
    //! \param[out] currentInfo currently collected info, which is complete when
    //! the method exits
    //! \return 0 if success, 1 if array dimension too large
    static int getArrayAccessInfo(ArraySubscriptExpr* fullExpr,
                                  std::stack<Expr*>* currentInfo);

   private:
    //! String representations of valid operators for use in constraints
    static std::map<BinaryOperatorKind, llvm::StringRef> operatorStrings;

    Utils() = delete;
};

}  // namespace spf_ie

#endif
