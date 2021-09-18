#ifndef SPFIE_DATAACCESSHANDLER_HPP
#define SPFIE_DATAACCESSHANDLER_HPP

#include <stack>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "clang/AST/Expr.h"

//! Maximum allowed array dimension (a safe estimate to avoid stack overflow)
#define MAX_ARRAY_DIM 50

//! Delimiter used to wrap data space names for easier renaming later
#define DATA_SPACE_DELIMITER "$"

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
  ArrayAccess(int64_t id, Expr *base, std::vector<Expr *> &indexes, bool isRead)
      : id(id), base(base), indexes(indexes), isRead(isRead) {}

  //! Get a string representation of the array access, like A(i,j).
  //! \param[in] potentialSubaccesses Other array accesses which may be used as indexes in this one
  std::string toString(const std::vector<ArrayAccess> &potentialSubaccesses) const;

  //! ID of original AST array access expression
  int64_t id;
  //! Base array being accessed
  Expr *base;
  //! Indexes accessed in the array
  std::vector<Expr *> indexes;
  //! Whether this access is a read or not (a write)
  bool isRead;
};

/*!
 * \struct DataAccessHandler
 *
 * \brief Handles data accesses (both reads and writes) for a statement
 */
struct DataAccessHandler {
public:
  //! Add all the arrays accessed in the expression as reads
  void processAsReads(Expr *expr);

  //! Add the array accessed as a write, and any accessed within it as reads
  void processAsWrite(ArraySubscriptExpr *expr);

  //! Make ArrayAccess and sub-accesses (recursively) from the given
  //! expression
  //! \param[in] expr Expression to process
  //! \param[in] isRead Whether this access is a read
  //! \param[in,out] existingAccesses Current list of sub-accesses; after
  //! processing completes, the last element will be the outermost access.
  static void buildDataAccess(
      ArraySubscriptExpr *expr, bool isRead,
      std::vector<ArrayAccess> &existingAccesses);

  //! Data spaces accessed
  std::unordered_set<std::string> dataSpaces;
  //! Array accesses
  std::vector<ArrayAccess> arrayAccesses;

private:
  //! Make an ArrayAccess from an ArraySubscriptExpr and add it to the
  //! appropriate map appropriate map
  void addDataAccess(ArraySubscriptExpr *expr, bool isRead);

  //! Do the recursive work of getting array access info
  //! \param[in] fullExpr array access to process
  //! \param[out] currentInfo currently collected info, which is complete when
  //! the method exits
  //! \return 0 if success, 1 if array dimension too large
  static int getArrayExprInfo(ArraySubscriptExpr *fullExpr,
                              std::stack<Expr *> *currentInfo);
};

}  // namespace spf_ie

#endif
