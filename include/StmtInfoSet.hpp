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

// one entry in an execution schedule, which may be a varible or a number
struct ScheduleVal {
    ScheduleVal(std::string var);
    ScheduleVal(int num);

    std::string var;
    int num;
    bool valueIsVar;
};

// contains associated information for a statement (iteration space and
// execution schedules)
struct StmtInfoSet {
    StmtInfoSet();
    StmtInfoSet(StmtInfoSet* other);

    // variables being iterated over
    std::vector<std::string> iterators;
    // constraints on iteration (inequalities and equalities)
    std::vector<std::shared_ptr<
        std::tuple<std::string, std::string, BinaryOperatorKind>>>
        constraints;
    // execution schedule, which begins at 0
    std::vector<std::shared_ptr<ScheduleVal>> schedule;

    // get a string representing the iteration space
    std::string getIterSpaceString();

    // get a string representing the execution schedule
    std::string getExecScheduleString();

    // move statement number forward in the schedule
    void advanceSchedule();

    // get the dimension of this execution schedule
    int getScheduleDimension() { return schedule.size(); }

    // zero-pad this execution schedule up to a certain dimension
    void zeroPadScheduleDimension(int dim);

    // enter* and exit* methods add iterators and constraints when entering a
    // new scope, remove when leaving the scope

    void enterFor(ForStmt* forStmt);

    void exitFor();

    void enterIf(IfStmt* ifStmt);

    void exitIf();

   private:
    // convenience function to add a new constraint from the given parameters
    void makeAndInsertConstraint(Expr* lower, Expr* upper,
                                 BinaryOperatorKind oper);

    void makeAndInsertConstraint(std::string lower, Expr* upper,
                                 BinaryOperatorKind oper);
};

}  // namespace pdfg_c

#endif
