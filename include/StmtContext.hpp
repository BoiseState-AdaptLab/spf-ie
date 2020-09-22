/*!
 * \file StmtContext.hpp
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

#include "ArrayAccess.hpp"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"

using namespace clang;

namespace spf_ie {

struct ScheduleVal;

/*!
 * \struct StmtContext
 *
 * \brief Contains associated information for a statement, such as iteration
 * space and execution schedule.
 */
struct StmtContext {
    StmtContext();

    //! Copy the information from an existing StmtContext.
    //! Preserves only information that builds up in nested contexts,
    //! excluding information that is only applicable per-statement.
    //! \param[in] other Existing StmtContext to copy
    StmtContext(StmtContext* other);

    //! Variables being iterated over
    std::vector<std::string> iterators;
    //! Constraints on iteration -- inequalities and equalities
    std::vector<std::shared_ptr<
        std::tuple<std::string, std::string, BinaryOperatorKind>>>
        constraints;
    //! Execution schedule
    std::vector<std::shared_ptr<ScheduleVal>> schedule;
    //! Array positions read from
    std::vector<ArrayAccess> dataReads;
    //! Array positions written to
    std::vector<ArrayAccess> dataWrites;

    //! Get a string representing the iteration space
    std::string getIterSpaceString();

    //! Get a string representing the execution schedule
    std::string getExecScheduleString();

    //! Get a string representing the data reads
    std::string getReadsString();

    //! Get a string representing the data writes
    std::string getWritesString();

    //! Move statement number forward in the execution schedule
    void advanceSchedule();

    //! Get the dimension of the execution schedule
    int getScheduleDimension() { return schedule.size(); }

    //! Zero-pad this execution schedule up to a certain dimension
    void zeroPadScheduleDimension(int dim);

    // enter* and exit* methods add iterators and constraints when entering a
    // new scope, remove when leaving the scope

    //! Add context information from a for loop
    void enterFor(ForStmt* forStmt);

    //! Remove context information from a for loop
    void exitFor();

    //! Add context information from an if statement
    void enterIf(IfStmt* ifStmt);

    //! Remove context information from an if statement
    void exitIf();

   private:
    //! Convenience function to add a new constraint from the given parameters
    void makeAndInsertConstraint(Expr* lower, Expr* upper,
                                 BinaryOperatorKind oper);

    //! Convenience function to add a new constraint from the given parameters
    void makeAndInsertConstraint(std::string lower, Expr* upper,
                                 BinaryOperatorKind oper);

    //! Get printable string representing the given data accesses
    std::string getDataAccessesString(
        std::vector<ArrayAccess>* accesses);
};

/*!
 * \struct ScheduleVal

 * \brief An entry of an execution schedule, which may be a variable or
 * simply a number.
 */
struct ScheduleVal {
    ScheduleVal(std::string var);
    ScheduleVal(int num);

    std::string var;
    int num;
    //! Whether this ScheduleVal contains a variable
    bool valueIsVar;
};

}  // namespace spf_ie

#endif
