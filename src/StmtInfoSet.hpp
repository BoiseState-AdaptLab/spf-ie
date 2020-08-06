#include <memory>
#include <tuple>
#include <vector>

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
    ScheduleVal(ValueDecl* var) : var(var), valueIsVar(true) {}
    ScheduleVal(int num) : num(num), valueIsVar(false) {}

    ValueDecl* var;
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
    std::vector<ValueDecl*> iterators;
    // constraints on iteration (inequalities and equalities)
    std::vector<std::shared_ptr<std::tuple<Expr*, Expr*, BinaryOperatorKind>>>
        constraints;
    // execution schedule, which begins at 0
    std::vector<std::shared_ptr<ScheduleVal>> schedule;

    std::string getIterSpaceString(ASTContext* Context) {
        std::string output;
        llvm::raw_string_ostream os(output);
        if (!constraints.empty()) {
            os << "{[";
            for (auto it = iterators.begin(); it != iterators.end(); ++it) {
                if (it != iterators.begin()) {
                    os << ",";
                }
                os << (*it)->getNameAsString();
            }
            os << "]: ";
            for (auto it = constraints.begin(); it != constraints.end(); ++it) {
                if (it != constraints.begin()) {
                    os << " && ";
                }
                BinaryOperatorKind bo = std::get<2>(**it);
                os << Utils::exprToString(std::get<0>(**it), Context) << " "
                   << Utils::binaryOperatorKindToString(std::get<2>(**it),
                                                        Context)
                   << " " << Utils::exprToString(std::get<1>(**it), Context);
            }
            os << "}";
        } else {
            os << "{[]}";
        }
        return os.str();
    }

    std::string getExecScheduleString(ASTContext* Context) {
        std::string output;
        llvm::raw_string_ostream os(output);
        os << "{[";
        for (auto it = iterators.begin(); it != iterators.end(); ++it) {
            if (it != iterators.begin()) {
                os << ",";
            }
            os << (*it)->getNameAsString();
        }
        os << "]->[";
        for (auto it = schedule.begin(); it != schedule.end(); ++it) {
            if (it != schedule.begin()) {
                os << ",";
            }
            if ((*it)->valueIsVar) {
                os << (*it)->var->getNameAsString();
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
        BinaryOperator* init = cast<BinaryOperator>(forStmt->getInit());
        makeAndInsertConstraint(init->getRHS(), init->getLHS(),
                                BinaryOperatorKind::BO_LE);
        BinaryOperator* cond = cast<BinaryOperator>(forStmt->getCond());
        makeAndInsertConstraint(cond->getLHS(), cond->getRHS(),
                                cond->getOpcode());
        ValueDecl* iterDecl =
            cast<DeclRefExpr>(init->getLHS()->IgnoreImplicit())->getDecl();
        iterators.push_back(iterDecl);
        schedule.push_back(std::make_shared<ScheduleVal>(iterDecl));
    }

    void exitFor() {
        constraints.pop_back();
        constraints.pop_back();
        iterators.pop_back();
        schedule.pop_back();
        schedule.pop_back();
    }

    void enterIf(IfStmt* ifStmt) {
        BinaryOperator* cond = cast<BinaryOperator>(ifStmt->getCond());
        makeAndInsertConstraint(cond->getLHS(), cond->getRHS(),
                                cond->getOpcode());
    }

    void exitIf() { constraints.pop_back(); }

   private:
    // convenience function to add a new constraint from the given parameters
    void makeAndInsertConstraint(Expr* lower, Expr* upper,
                                 BinaryOperatorKind oper) {
        constraints.push_back(
            std::make_shared<std::tuple<Expr*, Expr*, BinaryOperatorKind>>(
                lower, upper, oper));
    }
};
}  // namespace pdfg_c
