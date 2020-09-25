#include "StmtContext.hpp"

#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>
#include <vector>

#include "DataAccessHandler.hpp"
#include "Driver.hpp"
#include "ExecSchedule.hpp"
#include "Utils.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"

using namespace clang;

namespace spf_ie {

/* StmtContext */

StmtContext::StmtContext() {}

StmtContext::StmtContext(StmtContext* other) {
    iterators = other->iterators;
    constraints = other->constraints;
    schedule = other->schedule;
}

std::string StmtContext::getIterSpaceString() {
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
            os << std::get<0>(**it) << " "
               << Utils::binaryOperatorKindToString(std::get<2>(**it)) << " "
               << std::get<1>(**it);
        }
        os << "}";
    } else {
        os << "{[]}";
    }
    return os.str();
}

std::string StmtContext::getExecScheduleString() {
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
    for (auto it = schedule.scheduleTuple.begin();
         it != schedule.scheduleTuple.end(); ++it) {
        if (it != schedule.scheduleTuple.begin()) {
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

std::vector<std::string> StmtContext::getReadsStrings() {
    return getDataAccessStrings(&dataAccesses.reads);
}

std::vector<std::string> StmtContext::getWritesStrings() {
    return getDataAccessStrings(&dataAccesses.writes);
}

void StmtContext::enterFor(ForStmt* forStmt) {
    std::string error = std::string();
    std::string errorReason = std::string();

    // initializer
    std::string initVar;
    if (BinaryOperator* init = dyn_cast<BinaryOperator>(forStmt->getInit())) {
        makeAndInsertConstraint(init->getRHS(), init->getLHS(),
                                BinaryOperatorKind::BO_LE);
        initVar = Utils::stmtToString(init->getLHS());
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
    if (BinaryOperator* cond = dyn_cast<BinaryOperator>(forStmt->getCond())) {
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
            BinaryOperator* secondOp = cast<BinaryOperator>(incOper->getRHS());
            // Get variable being incremented
            std::string iterStr = Utils::stmtToString(incOper->getLHS());
            if (secondOp->getOpcode() == BO_Add) {
                // Get our lh and rh expression, also in string form
                Expr* lhs = secondOp->getLHS();
                Expr* rhs = secondOp->getRHS();
                std::string lhsStr = Utils::stmtToString(lhs);
                std::string rhsStr = Utils::stmtToString(rhs);
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
        schedule.pushValue(initVar);
    }
}

void StmtContext::exitFor() {
    constraints.pop_back();
    constraints.pop_back();
    iterators.pop_back();
    schedule.popValue();
    schedule.popValue();
}

void StmtContext::enterIf(IfStmt* ifStmt) {
    if (BinaryOperator* cond = dyn_cast<BinaryOperator>(ifStmt->getCond())) {
        makeAndInsertConstraint(cond->getLHS(), cond->getRHS(),
                                cond->getOpcode());
    } else {
        Utils::printErrorAndExit(
            "If statement condition must be a binary operation", ifStmt);
    }
}

void StmtContext::exitIf() { constraints.pop_back(); }

void StmtContext::makeAndInsertConstraint(Expr* lower, Expr* upper,
                                          BinaryOperatorKind oper) {
    makeAndInsertConstraint(Utils::stmtToString(lower), upper, oper);
}

void StmtContext::makeAndInsertConstraint(std::string lower, Expr* upper,
                                          BinaryOperatorKind oper) {
    constraints.push_back(
        std::make_shared<
            std::tuple<std::string, std::string, BinaryOperatorKind>>(
            lower, Utils::stmtToString(upper), oper));
}

std::vector<std::string> StmtContext::getDataAccessStrings(
    std::map<std::string, ArrayAccess>* accesses) {
    std::ostringstream itersStrStream;
    itersStrStream << "[";
    for (auto it = iterators.begin(); it != iterators.end(); ++it) {
        if (it != iterators.begin()) {
            itersStrStream << ",";
        }
        itersStrStream << *it;
    }
    itersStrStream << "]";
    std::string itersStr = itersStrStream.str();

    // build strings
    std::vector<std::string> strings;
    for (auto it = accesses->begin(); it != accesses->end(); ++it) {
        std::ostringstream os;
        os << "{" << itersStr << "->[";
        for (auto it_indexes = it->second.indexes.begin();
             it_indexes != it->second.indexes.end(); ++it_indexes) {
            if (it_indexes != it->second.indexes.begin()) {
                os << ",";
            }
            os << Utils::stmtToString(*it_indexes);
        }
        os << "]}";
        strings.push_back(os.str());
    }

    return strings;
}

}  // namespace spf_ie
