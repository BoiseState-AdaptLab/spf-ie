#ifndef SPFIE_SPFCOMPUTATIONBUILDER_HPP
#define SPFIE_SPFCOMPUTATIONBUILDER_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "PositionContext.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "iegenlib.h"

using namespace clang;

namespace spf_ie {

/*!
 * \class ComputationBuilder
 *
 * \brief Class handling building up the sparse polyhedral model for a function
 *
 * Contains the entry point for function processing. Recursively visits each
 * statement in the source.
 */
class ComputationBuilder {
public:
  ComputationBuilder();

  //! Entry point for building top-level Computation.
  //! Gathers information about the function's
  //! statements and data accesses into a Computation.
  //! \param[in] funcDecl Function declaration to process
  Computation *buildComputationFromFunction(
      FunctionDecl *funcDecl);

  //! Computations referenced from any others, stored for potential re-use
  static std::map<std::string, Computation *> subComputations;

  //! Context information about the position we're currently at.
  //! Updated to the most recent statement in the Computation currently being processed.
  static PositionContext *positionContext;

  //! Names of reserved (standard library) functions that should not be inlined
  const static std::unordered_set<std::string> reservedFuncNames;

private:
  //! Top-level Computation being built up
  Computation *computation;
  //! Whether a return Stmt has been hit in this function
  bool haveFoundAReturn = false;
  //! Declarations found, which may need to be consulted for type info later
  std::map<std::string, QualType> varDecls;
  //! Data accesses for statement currently being processed
  DataAccessHandler dataAccesses;
  //! String replacement rules to be applied for current statement, stored as from->to.
  //! Currently only used to replace calls to inline functions with their return values.
  std::map<std::string, std::string> stmtSourceCodeReplacements;

  //! Process the body of a control structure, such as a for loop
  //! \param[in] stmt Body statement (which may be compound) to process
  void processBody(clang::Stmt *stmt);

  //! Process one statement, recursing on compound statements
  //! \param[in] stmt Statement to process
  void processSingleStmt(clang::Stmt *stmt);

  //! Add info about a completed statement to the builder
  //! \param[in] stmt Completed statement to save
  void addStmt(clang::Stmt *stmt);

  //! Handle a return statement in the function
  void processReturnStmt(clang::ReturnStmt *returnStmt);

  //! Inline a nested function call and get its return value, if any
  std::string inlineFunctionCall(CallExpr *callExpr);

  //! Perform processing on a compound expression, including inlining any function calls.
  //! \param[in] expr Expression to process
  //! \param[in] processReads If true, also add any referenced data spaces as reads,
  //! including those returned from inlined functions
  void processComplexExpr(Expr *expr, bool processReads);
};

}  // namespace spf_ie

#endif
