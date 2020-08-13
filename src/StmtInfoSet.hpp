#include <memory>
#include <tuple>
#include <vector>

#include "PDFGDriver.hpp"
#include "Utils.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/StringRef.h"

using namespace clang;

namespace pdfg_c {

// one entry in an execution schedule, which may be a varible or a number
struct ScheduleVal {
    ScheduleVal(std::string var) : var(var), valueIsVar(true) {}
    ScheduleVal(int num) : num(num), valueIsVar(false) {}

    std::string var;
    int num;
    bool valueIsVar;
};

// contains associated information for a statement (iteration space and
// execution schedules)
struct StmtInfoSet {
    StmtInfoSet() {}
    StmtInfoSet(StmtInfoSet* other) {
        iterators = other->iterators;
        constraints = other->constraints;
        schedule = other->schedule;
    }

    // variables being iterated over
    std::vector<std::string> iterators;
    // constraints on iteration (inequalities and equalities)
    std::vector<std::shared_ptr<
        std::tuple<std::string, std::string, BinaryOperatorKind>>>
        constraints;
    // execution schedule, which begins at 0
    std::vector<std::shared_ptr<ScheduleVal>> schedule;

    // get a string representing the iteration space
    std::string getIterSpaceString() {
        std::string output;
        llvm::raw_string_ostream os(output);
        if (!constraints.empty()) {
            os << "{[";
            for (auto it = iterators.begin(); it != iterators.end(); ++it) {
                if (it != iterators.begin()) {
                    os << ",";
                }
                os << *it;
            }
            os << "]: ";
            for (auto it = constraints.begin(); it != constraints.end(); ++it) {
                if (it != constraints.begin()) {
                    os << " and ";
                }
                BinaryOperatorKind bo = std::get<2>(**it);
                os << std::get<0>(**it) << " "
                   << Utils::binaryOperatorKindToString(std::get<2>(**it))
                   << " " << std::get<1>(**it);
            }
            os << "}";
        } else {
            os << "{[]}";
        }
        return os.str();
    }

    // get a string representing the execution schedule
    std::string getExecScheduleString() {
        std::string output;
        llvm::raw_string_ostream os(output);
        os << "{[";
        for (auto it = iterators.begin(); it != iterators.end(); ++it) {
            if (it != iterators.begin()) {
                os << ",";
            }
            os << *it;
        }
        os << "]->[";
        for (auto it = schedule.begin(); it != schedule.end(); ++it) {
            if (it != schedule.begin()) {
                os << ",";
            }
            if ((*it)->valueIsVar) {
                os << (*it)->var;
            } else {
                os << (*it)->num;
            }
        }
        os << "]}";
        return os.str();
    }

    // move statement number forward in the schedule
    void advanceSchedule() {
        if (schedule.empty() || schedule.back()->valueIsVar) {
            schedule.push_back(std::make_shared<ScheduleVal>(0));
        } else {
            std::shared_ptr<ScheduleVal> top = schedule.back();
            schedule.pop_back();
            schedule.push_back(std::make_shared<ScheduleVal>(top->num + 1));
        }
    }

    // get the dimension of this execution schedule
    int getScheduleDimension() { return schedule.size(); }

    // zero-pad this execution schedule up to a certain dimension
    void zeroPadScheduleDimension(int dim) {
        for (int i = getScheduleDimension(); i < dim; ++i) {
            schedule.push_back(std::make_shared<ScheduleVal>(0));
        }
    }

    // enter* and exit* methods add iterators and constraints when entering a
    // new scope, remove when leaving the scope

    void enterFor(ForStmt* forStmt) {
        std::string error = std::string();
        std::string errorReason = std::string();

        // initializer
        std::string initVar;
        if (BinaryOperator* init =
                dyn_cast<BinaryOperator>(forStmt->getInit())) {
            makeAndInsertConstraint(init->getRHS(), init->getLHS(),
                                    BinaryOperatorKind::BO_LE);
            initVar = Utils::exprToString(init->getLHS());
        } else if (DeclStmt* init = dyn_cast<DeclStmt>(forStmt->getInit())) {
            if (VarDecl* initDecl = dyn_cast<VarDecl>(init->getSingleDecl())) {
                makeAndInsertConstraint(initDecl->getNameAsString(),
                                        initDecl->getInit(),
                                        BinaryOperatorKind::BO_LE);
                initVar = initDecl->getNameAsString();
            } else {
                error = "initializer";
                errorReason = "declarative initializer must declare a variable";
            }
        } else {
            error = "initializer";
            errorReason = "must initialize iterator";
        }

        // condition
        if (BinaryOperator* cond =
                dyn_cast<BinaryOperator>(forStmt->getCond())) {
            makeAndInsertConstraint(cond->getLHS(), cond->getRHS(),
                                    cond->getOpcode());
        } else {
            error = "condition";
            errorReason = "must be a binary operation";
        }

        // increment
        bool validIncrement = false;
        Expr* inc = forStmt->getInc();
        UnaryOperator* unaryIncOper = dyn_cast<UnaryOperator>(inc);
        if (unaryIncOper && unaryIncOper->isIncrementOp()) {
            // simple increment, ++i or i++
            validIncrement = true;
        } else if (BinaryOperator* incOper = dyn_cast<BinaryOperator>(inc)) {
            BinaryOperatorKind oper = incOper->getOpcode();
            if (oper == BO_AddAssign || oper == BO_SubAssign) {
                // operator is += or -=
                Expr::EvalResult result;
                if (incOper->getRHS()->EvaluateAsInt(result, *Context)) {
                    llvm::APSInt incVal = result.Val.getInt();
                    validIncrement = (oper == BO_AddAssign && incVal == 1) ||
                                     (oper == BO_SubAssign && incVal == -1);
                }
            } else if (oper == BO_Assign &&
                       isa<BinaryOperator>(incOper->getRHS())) {
                // Operator is '='
                // This is the rhs of the increment statement
                // (e.g. with i = i + 1, this is i + 1)
                BinaryOperator* secondOp =
                    cast<BinaryOperator>(incOper->getRHS());
                // Get variable being incremented
                std::string iterStr = Utils::exprToString(incOper->getLHS());
                if (secondOp->getOpcode() == BO_Add) {
                    // Get our lh and rh expression, also in string form
                    Expr* lhs = secondOp->getLHS();
                    Expr* rhs = secondOp->getRHS();
                    std::string lhsStr = Utils::exprToString(lhs);
                    std::string rhsStr = Utils::exprToString(rhs);
                    Expr::EvalResult result;
                    // one side must be iter var, other must be 1
                    validIncrement = (lhsStr.compare(iterStr) == 0 &&
                                      rhs->EvaluateAsInt(result, *Context) &&
                                      result.Val.getInt() == 1) ||
                                     (rhsStr.compare(iterStr) == 0 &&
                                      lhs->EvaluateAsInt(result, *Context) &&
                                      result.Val.getInt() == 1);
                }
            }
        }
        if (!validIncrement) {
            error = "increment";
            errorReason = "must increase iterator by 1";
        }

        if (!error.empty()) {
            Utils::printErrorAndExit(
                "Invalid " + error + " in for loop -- " + errorReason, forStmt);
        } else {
            iterators.push_back(initVar);
            schedule.push_back(std::make_shared<ScheduleVal>(initVar));
        }
    }

    void exitFor() {
        constraints.pop_back();
        constraints.pop_back();
        iterators.pop_back();
        schedule.pop_back();
        schedule.pop_back();
    }

    void enterIf(IfStmt* ifStmt) {
        if (BinaryOperator* cond =
                dyn_cast<BinaryOperator>(ifStmt->getCond())) {
            makeAndInsertConstraint(cond->getLHS(), cond->getRHS(),
                                    cond->getOpcode());
        } else {
            Utils::printErrorAndExit(
                "If statement condition must be a binary operation", ifStmt);
        }
    }

    void exitIf() { constraints.pop_back(); }

   private:
    // convenience function to add a new constraint from the given parameters
    void makeAndInsertConstraint(Expr* lower, Expr* upper,
                                 BinaryOperatorKind oper) {
        makeAndInsertConstraint(Utils::exprToString(lower), upper, oper);
    }

    void makeAndInsertConstraint(std::string lower, Expr* upper,
                                 BinaryOperatorKind oper) {
        constraints.push_back(
            std::make_shared<
                std::tuple<std::string, std::string, BinaryOperatorKind>>(
                lower, Utils::exprToString(upper), oper));
    }
};

}  // namespace pdfg_c
