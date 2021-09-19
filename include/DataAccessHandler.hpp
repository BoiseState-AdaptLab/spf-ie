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
  ArrayAccess(std::string arrayName, int64_t sourceId, std::vector<Expr *> &indexes, bool isRead)
      : arrayName(arrayName), sourceId(sourceId), indexes(indexes), isRead(isRead) {}

  //! Get a string representation of the array access, like A(i,j).
  //! \param[in] potentialSubaccesses Other array accesses here, which may be used as indexes in this one
  std::string toString(const std::vector<ArrayAccess> &potentialSubaccesses) const;

  //! ID of original ArraySubscriptExpr node
  int64_t sourceId;
  //! Name of base (outermost) array being accessed
  std::string arrayName;
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

  //! Make all ArrayAccesses, including subaccesses, from the given expression
  //! \param[in] fullExpr Expression to process
  //! \param[in] isRead Whether this access is a read
  static std::vector<ArrayAccess> buildDataAccesses(
      ArraySubscriptExpr *fullExpr, bool isRead);

  //! Data spaces accessed
  std::unordered_set<std::string> dataSpaces;
  //! Array accesses
  std::vector<ArrayAccess> arrayAccesses;

private:
  //! Make and store an ArrayAccess from an ArraySubscriptExpr, as the given access type
  void processAsDataAccess(ArraySubscriptExpr *fullExpr, bool isRead);

  //! Do recursive work of building ArrayAccesses
  //! \param[in] fullExpr Expression to process
  //! \param[in] isRead Whether this access is a read
  //! \param[out] existingAccesses Current list of sub-accesses; after
  //! processing completes, the last element will be the outermost access.
  static void doBuildDataAccessesWork(ArraySubscriptExpr *fullExpr,
                                      bool isRead,
                                      std::vector<ArrayAccess> &existingAccesses);

  //! Do the recursive work of getting array access info
  //! \param[in] fullExpr array access to process
  //! \param[out] currentInfo currently collected info, which is complete when
  //! the method exits
  //! \return false if an error occurred, true otherwise
  static int getArrayExprInfo(ArraySubscriptExpr *fullExpr,
                              std::stack<Expr *> *currentInfo);
};

}  // namespace spf_ie

#endif
