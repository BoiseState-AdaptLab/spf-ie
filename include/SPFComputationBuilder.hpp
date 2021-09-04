#ifndef SPFIE_SPFCOMPUTATIONBUILDER_HPP
#define SPFIE_SPFCOMPUTATIONBUILDER_HPP

#include <memory>
#include <string>
#include <vector>

#include "StmtContext.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "iegenlib.h"

using namespace clang;

namespace spf_ie {

/*!
 * \class SPFComputationBuilder
 *
 * \brief Class handling building up the sparse polyhedral model for a function
 *
 * Contains the entry point for function processing. Recursively visits each
 * statement in the source.
 */
class SPFComputationBuilder {
public:
  SPFComputationBuilder();
  //! Entry point for each function; gather information about its
  //! statements and data accesses into an Computation
  //! \param[in] funcDecl Function declaration to process
  std::unique_ptr<iegenlib::Computation> buildComputationFromFunction(
	  FunctionDecl *funcDecl);

private:
  //! Number of the statement currently being processed
  unsigned int stmtNumber{};
  //! The length of the longest schedule tuple
  int largestScheduleDimension{};
  //! The information about the context we are currently in, which is
  //! copied for completed statements
  StmtContext currentStmtContext;
  //! Context information attached to each completed statement
  std::vector<StmtContext> stmtContexts;
  //! Computation being built up
  std::unique_ptr<iegenlib::Computation> computation;

  //! Process the body of a control structure, such as a for loop
  //! \param[in] stmt Body statement (which may be compound) to process
  void processBody(clang::Stmt *stmt);

  //! Process one statement, recursing on compound statements
  //! \param[in] stmt Statement to process
  void processSingleStmt(clang::Stmt *stmt);

  //! Add info about a completed statement to the builder
  //! \param[in] stmt Completed statement to save
  void addStmt(clang::Stmt *stmt);
};

}  // namespace spf_ie

#endif
