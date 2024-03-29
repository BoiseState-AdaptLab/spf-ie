/*!
 * \file PositionContext.hpp
 *
 * \brief Structs representing the information associated with a statement
 * based on its position in the code and surrounding control structures
 *
 * \author Anna Rift
 * \author Aaron Orenstein
 */

#ifndef SPFIE_STMTCONTEXT_HPP
#define SPFIE_STMTCONTEXT_HPP

#include <memory>
#include <stack>
#include <string>
#include <tuple>
#include <vector>

#include "DataAccessHandler.hpp"
#include "ExecSchedule.hpp"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"

using namespace clang;

namespace spf_ie {

/*!
 * \struct PositionContext
 *
 * \brief Contains information associated with a statement position, such as iteration
 * domain and execution schedule.
 */
struct PositionContext {
  PositionContext();

  //! Variables being iterated over
  std::vector<std::string> iterators;
  //! Constraints on iteration -- inequalities and equalities
  std::vector<std::shared_ptr<
      std::tuple<std::string, std::string, BinaryOperatorKind>>>
      constraints;
  //! Execution schedule
  ExecSchedule schedule;
  //! How deeply nested within compound structures this position is
  unsigned int nestLevel = 0;

  //! Data spaces which are held invariant in the current context, grouped
  //! by the loop that they are invariant in
  std::vector<std::vector<std::string>> invariants;

  //! Get a string representing the iteration space
  std::string getIterSpaceString();

  //! Get a string representing the execution schedule
  std::string getExecScheduleString();

  //! Get a string representing the given data access
  std::string getDataAccessString(DataAccess *);

  //! Check whether the given name is an iterator in this context
  bool isIteratorName(const std::string &varName);

  // enter* and exit* methods add iterators and constraints when entering a
  // new scope, remove when leaving the scope

  //! Add context information from a for loop
  void enterFor(ForStmt *forStmt);

  //! Remove context information from a for loop
  void exitFor();

  //! Add context information from an if statement
  //! \param[in] ifStmt If statement to use
  //! \param[in] invert Whether to invert the if condition (for use in
  //! else clauses)
  void enterIf(IfStmt *ifStmt, bool invert = false);

  //! Remove context information from an if statement
  void exitIf();

private:
  //! Convenience function to add a new constraint from the given parameters
  void makeAndInsertConstraint(Expr *lower, Expr *upper,
                               BinaryOperatorKind oper);

  //! Convenience function to add a new constraint from the given parameters
  void makeAndInsertConstraint(std::string lower, Expr *upper,
                               BinaryOperatorKind oper);

  //! Get the source code of an expression, with array accesses changed to
  //! function calls (for example, "i < A[i]" becomes "i < A(i)")
  static std::string exprToStringWithSafeArrays(Expr *expr);

  //! Get the tuple of iterators as a string, for use in other to-string
  //! methods. Output like "[i,j,k]"
  std::string getItersTupleString();
};

}  // namespace spf_ie

#endif
