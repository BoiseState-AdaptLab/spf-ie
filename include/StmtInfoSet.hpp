/*!
 * \file StmtInfoSet.hpp
 *
 * \brief Structs representing all information associated with a statement.
 *
 * \author Anna Rift
 * \author Aaron Orenstein
 */

#ifndef PDFG_STMTINFOSET_HPP
#define PDFG_STMTINFOSET_HPP

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"

using namespace clang;

namespace pdfg_c {

struct ScheduleVal;

/*!
 * \struct StmtInfoSet
 *
 * \brief Contains associated information for a statement, such as iteration
 * space and execution schedule.
 */
struct StmtInfoSet {
    StmtInfoSet();

    //! Copy the information from an existing StmtInfoSet
    //! \param[in] other Existing StmtInfoSet to copy
    StmtInfoSet(StmtInfoSet* other);

    //! Variables being iterated over
    std::vector<std::string> iterators;
    //! Constraints on iteration -- inequalities and equalities
    std::vector<std::shared_ptr<
        std::tuple<std::string, std::string, BinaryOperatorKind>>>
        constraints;
    //! Execution schedule
    std::vector<std::shared_ptr<ScheduleVal>> schedule;

    //! Get a string representing the iteration space
    std::string getIterSpaceString();

    //! Get a string representing the execution schedule
    std::string getExecScheduleString();

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

}  // namespace pdfg_c

#endif
