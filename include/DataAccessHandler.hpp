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

using namespace clang;

namespace spf_ie {

/*!
 * \struct DataAccess
 *
 * \brief Representation of a data access
 *
 * Used partly because the AST's representation of a multidimensional array access
 * is difficult to work with for our purposes. This can represent either an array
 * subscript access or scalar access.
 */
struct DataAccess {
  DataAccess(std::string name, int64_t sourceId, bool isRead, bool isArrayAccess, std::vector<Expr *> indexes)
      : name(name), sourceId(sourceId), isRead(isRead), isArrayAccess(isArrayAccess), indexes(indexes) {}

  //! Get a string representation of the data access, like A(i,j) for an array or x for a scalar.
  //! \param[in] potentialSubaccesses Other array accesses here, which may be used as indexes in this one
  std::string toString(const std::vector<DataAccess> &potentialSubaccesses) const;

  //! Name of base variable being accessed. In the case of an array, this will be the outermost access.
  std::string name;
  //! ID of original AST node this represents
  int64_t sourceId;
  //! Whether this access is a read or not (a write)
  bool isRead;
  //! Whether this access is an array (non-scalar) access.
  bool isArrayAccess;
  //! Indexes accessed in the data space
  std::vector<Expr *> indexes;
};

/*!
 * \struct DataAccessHandler
 *
 * \brief Handles data accesses (both reads and writes) for a statement.
 *
 * When referring to arrays, the 'access' is the access to the outer array in the subscript access, and sub-accesses are
 * subscript accesses used as the subscripts in outer arrays.
 */
struct DataAccessHandler {
public:
  //! Add all the arrays accessed in the expression as reads
  void processComplexExprAsReads(Expr *expr);

  //! Add the space accessed as a write, and any sub-accesses as reads
  void processExprAsWrite(Expr *expr);

  //! Add a scalar's name as a write DataAccess.
  void processWriteToScalarName(const std::string name);

  //! Make all data accesses, including subaccesses, from the given expression
  //! \param[in] fullExpr Expression to process
  //! \param[in] isRead Whether this access is a read
  static std::vector<DataAccess> makeDataAccessesFromExpr(
      Expr *fullExpr, bool isRead);

  //! Retrieve "all" data accesses, from left to right, contained in an
  //! expression.
  //! Recurses into BinaryOperators.
  //! \param[in] expr Expression to process
  //! \param[out] currentList List of accesses
  static void collectAllDataAccessesInCompoundExpr(
      Expr *expr, std::vector<Expr *> &currentList);

  //! Data accesses
  std::vector<DataAccess> stmtDataAccesses;
  //! Data spaces accessed
  std::unordered_set<std::string> dataSpacesAccessed;

private:
  //! Make and store a DataAccess from an expression, plus sub-accesses, as the given access type
  //! \param[in] fullExpr Expression to process. Should just be a variable reference or array subscript expression.
  //! \param[in] isRead Whether this access is a read
  void processSingleAccessExpr(Expr *fullExpr, bool isRead);

  //! Do recursive work of building a DataAccess from an array
  //! \param[in] fullExpr Expression to process
  //! \param[in] isRead Whether this access is a read
  //! \param[out] existingAccesses Current list of sub-accesses; after
  //! processing completes, the last element will be the outermost access.
  static void doBuildArrayAccessWork(ArraySubscriptExpr *fullExpr,
                                     bool isRead,
                                     std::vector<DataAccess> &existingAccesses);

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
