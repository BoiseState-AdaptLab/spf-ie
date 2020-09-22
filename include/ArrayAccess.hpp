#ifndef SPFIE_ARRAYACCESS_HPP
#define SPFIE_ARRAYACCESS_HPP

#include <stack>
#include <string>
#include <vector>

#include "clang/AST/Expr.h"

//! Maximum allowed array dimension (a safe estimate to avoid stack overflow)
#define MAX_ARRAY_DIM 50

using namespace clang;

namespace spf_ie {

/*!
 * \struct ArrayAccess
 *
 * \brief Representation of an array (subscript) access
 *
 * Used because the AST's representation of a multidimensional array access
 * is difficult to work with for our purposes.
 */
struct ArrayAccess {
    ArrayAccess(Expr* base, std::vector<Expr*> indexes)
        : base(base), indexes(indexes) {}

    //! Make an ArrayAccess from an AST ArraySubscriptExpr
    static ArrayAccess makeArrayAccess(ArraySubscriptExpr* expr);

    //! Get a string representation of the array access, like A(i,j)
    std::string toString();

    //! Base array being accessed
    Expr* base;
    //! Indexes accessed in the array
    std::vector<Expr*> indexes;

   private:
    //! Do the recursive work of getting array access info
    //! \param[in] fullExpr array access to process
    //! \param[out] currentInfo currently collected info, which is complete when
    //! the method exits
    //! \return 0 if success, 1 if array dimension too large
    static int getArrayExprInfo(ArraySubscriptExpr* fullExpr,
                                std::stack<Expr*>* currentInfo);
};

}  // namespace spf_ie

#endif
