#ifndef SPFIE_SPFCOMPUTATIONBUILDER_HPP
#define SPFIE_SPFCOMPUTATIONBUILDER_HPP

#include <string>
#include <vector>

#include "SPFComputation.hpp"
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
    explicit SPFComputationBuilder();
    //! Entry point for each function; gathers information about its
    //! statements
    //! \param[in] funcDecl Function declaration to process
    void processFunction(FunctionDecl* funcDecl);

    //! Print collected information to standard output
    void printInfo();

   private:
    //! Number of the statement currently being processed
    unsigned int stmtNumber;
    //! The information about the context we are currently in, which is
    //! copied for completed statements
    StmtContext currentStmtContext;
    //! The length of the longest schedule tuple
    int largestScheduleDimension;
    //! SPFComputation being built up
    SPFComputation computation;

    //! Process the body of a control structure, such as a for loop
    //! \param[in] stmt Body statement (which may be compound) to process
    void processBody(Stmt* stmt);

    //! Process one statement, recursing on compound statements
    //! \param[in] stmt Statement to process
    void processSingleStmt(Stmt* stmt);

    //! Add info about a completed statement to the builder
    //! \param[in] stmt Completed statement to save
    void addStmt(Stmt* stmt);
};

}  // namespace spf_ie

#endif
