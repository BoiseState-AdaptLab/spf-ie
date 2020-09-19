#ifndef SPFIE_SPFFUNCBUILDER_HPP
#define SPFIE_SPFFUNCBUILDER_HPP

#include <string>
#include <vector>

#include "StmtInfoSet.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "iegenlib.h"

using namespace clang;

namespace spf_ie {

/*!
 * \class SPFFuncBuilder
 *
 * \brief Class handling building up the sparse polyhedral model for a function
 *
 * Contains the entry point for function processing. Recursively visits each
 * statement in the source.
 */
class SPFFuncBuilder {
   public:
    explicit SPFFuncBuilder();
    //! Entry point for each function; gathers information about its
    //! statements
    //! \param[in] funcDecl Function declaration to process
    void processFunction(FunctionDecl* funcDecl);

    //! Print collected information to standard output
    void printInfo();

   private:
    //! Name of the function being processed
    std::string functionName;
    //! Statements that have completed processing
    std::vector<Stmt*> stmts;
    //! Context information attached to each completed statement
    std::vector<StmtInfoSet> stmtInfoSets;
    //! The information about the context we are currently in, which is
    //! copied for completed statements
    StmtInfoSet currentStmtInfoSet;
    //! The length of the longest schedule tuple
    int largestScheduleDimension;

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

struct IEGenDataAccess {
    IEGenDataAccess(std::string stmt, std::string accessExpression,
                   std::string arrayAccessed, iegenlib::Relation access)
        : stmt(stmt),
          accessExpression(accessExpression),
          arrayAccessed(arrayAccessed),
          access(access) {}

    std::string stmt;
    std::string accessExpression;
    std::string arrayAccessed;
    iegenlib::Relation access;
};

}  // namespace spf_ie

#endif
