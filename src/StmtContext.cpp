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
#include "iegenlib.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"

using namespace clang;

namespace spf_ie {

/* StmtContext */

StmtContext::StmtContext() = default;

std::string StmtContext::getIterSpaceString() {
  std::ostringstream os;
  os << "{" << getItersTupleString();
  if (!constraints.empty()) {
    os << ": ";
    for (const auto &it: constraints) {
      if (it != *constraints.begin()) {
        os << " and ";
      }
      os << std::get<0>(*it) << " "
         << Utils::binaryOperatorKindToString(std::get<2>(*it)) << " "
         << std::get<1>(*it);
    }
  }
  os << "}";
  return os.str();
}

std::string StmtContext::getExecScheduleString() {
  std::ostringstream os;
  os << "{" << getItersTupleString() << "->[";
  if (schedule.scheduleTuple.empty()) {
    os << "0";
  } else {
    for (const auto &it: schedule.scheduleTuple) {
      if (it != *schedule.scheduleTuple.begin()) {
        os << ",";
      }
      if (it->valueIsVar) {
        os << it->var;
      } else {
        os << it->num;
      }
    }
  }
  os << "]}";
  return os.str();
}

std::string StmtContext::getDataAccessString(ArrayAccess *access) {
  std::ostringstream os;
  std::vector<std::pair<std::string, std::string>> constraintsToAdd;
  os << "{" << getItersTupleString() << "->[";
  if (access->indexes.empty()) {
    os << "0";
  } else {
    for (const auto &it: access->indexes) {
      if (it != *access->indexes.begin()) {
        os << ",";
      }
      std::vector<ArraySubscriptExpr *> subAccesses;
      Utils::getExprArrayAccesses(it, subAccesses);
      if (!subAccesses.empty()) {
        std::string replacementName = Utils::getVarReplacementName();
        os << replacementName;
        constraintsToAdd.emplace_back(replacementName, exprToStringWithSafeArrays(it));
      } else if (isa<DeclRefExpr>(it->IgnoreParenImpCasts())) {
        os << Utils::stmtToString(it);
      } else {
        // if the expression is not a nested access or single variable
        // simply assign it to a replacement variable and use that
        std::string replacementName = Utils::getVarReplacementName();
        os << replacementName;
        constraintsToAdd.emplace_back(replacementName, Utils::stmtToString(it));
      }
    }
  }
  os << "]";
  if (!constraintsToAdd.empty()) {
    os << ": ";
    for (const auto &constraint: constraintsToAdd) {
      if (constraint != *constraintsToAdd.begin()) {
        os << " && ";
      }
      os << constraint.first << " = " << constraint.second;
    }
  }
  os << "}";
  return os.str();
}

void StmtContext::enterFor(ForStmt *forStmt) {
  std::string error;
  std::string errorReason;

  // initializer
  std::string initVar;
  if (auto *init = dyn_cast<BinaryOperator>(forStmt->getInit())) {
    makeAndInsertConstraint(init->getRHS(), init->getLHS(),
                            BinaryOperatorKind::BO_LE);
    initVar = Utils::stmtToString(init->getLHS());
  } else if (auto *init = dyn_cast<DeclStmt>(forStmt->getInit())) {
    if (auto *initDecl = dyn_cast<VarDecl>(init->getSingleDecl())) {
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
  if (auto *cond = dyn_cast<BinaryOperator>(forStmt->getCond())) {
    makeAndInsertConstraint(cond->getLHS(), cond->getRHS(),
                            cond->getOpcode());
    // add any data spaces accessed in the condition to loop invariants
    std::vector<std::string> newInvariants;
    std::vector<ArraySubscriptExpr *> accessExprs;
    Utils::getExprArrayAccesses(cond->getLHS(), accessExprs);
    Utils::getExprArrayAccesses(cond->getRHS(), accessExprs);
    std::vector<ArrayAccess> accesses;
    for (const auto &accessExpr: accessExprs) {
      auto additionalAccesses = DataAccessHandler::buildDataAccesses(accessExpr, true);
      accesses.insert(accesses.end(), additionalAccesses.begin(), additionalAccesses.end());
    }
    for (const auto &access: accesses) {
      newInvariants.push_back(
          access.arrayName);
    }
    invariants.push_back(newInvariants);
  } else {
    error = "condition";
    errorReason = "must be a binary operation";
  }

  // increment
  bool validIncrement = false;
  Expr *inc = forStmt->getInc();
  auto *unaryIncOper = dyn_cast<UnaryOperator>(inc);
  if (unaryIncOper && unaryIncOper->isIncrementOp()) {
    // simple increment, ++i or i++
    validIncrement = true;
  } else if (auto *incOper = dyn_cast<BinaryOperator>(inc)) {
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
      auto *secondOp = cast<BinaryOperator>(incOper->getRHS());
      // Get variable being incremented
      std::string iterStr = Utils::stmtToString(incOper->getLHS());
      if (secondOp->getOpcode() == BO_Add) {
        // Get our lh and rh expression, also in string form
        Expr *lhs = secondOp->getLHS();
        Expr *rhs = secondOp->getRHS();
        std::string lhsStr = Utils::stmtToString(lhs);
        std::string rhsStr = Utils::stmtToString(rhs);
        Expr::EvalResult result;
        // one side must be iter var, other must be 1
        validIncrement = (lhsStr == iterStr &&
            rhs->EvaluateAsInt(result, *Context) &&
            result.Val.getInt() == 1) ||
            (rhsStr == iterStr &&
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
    schedule.pushValue(ScheduleVal(initVar));
  }
}

void StmtContext::exitFor() {
  constraints.pop_back();
  constraints.pop_back();
  iterators.pop_back();
  schedule.popValue();
  schedule.popValue();
  invariants.pop_back();
}

void StmtContext::enterIf(IfStmt *ifStmt, bool invert) {
  if (auto *cond = dyn_cast<BinaryOperator>(ifStmt->getCond())) {
    makeAndInsertConstraint(
        cond->getLHS(), cond->getRHS(),
        (invert ? BinaryOperator::negateComparisonOp(cond->getOpcode())
                : cond->getOpcode()));
  } else {
    Utils::printErrorAndExit(
        "If statement condition must be a binary operation", ifStmt);
  }
}

void StmtContext::exitIf() { constraints.pop_back(); }

void StmtContext::makeAndInsertConstraint(Expr *lower, Expr *upper,
                                          BinaryOperatorKind oper) {
  makeAndInsertConstraint(exprToStringWithSafeArrays(lower), upper, oper);
}

void StmtContext::makeAndInsertConstraint(std::string lower, Expr *upper,
                                          BinaryOperatorKind oper) {
  if (oper == BinaryOperatorKind::BO_NE) {
    Utils::printErrorAndExit(
        "Not-equal conditions are unsupported by SPF: in condition " +
            lower + " != " + Utils::stmtToString(upper),
        upper);
  }
  constraints.push_back(
      std::make_shared<
          std::tuple<std::string, std::string, BinaryOperatorKind>>(
          lower, exprToStringWithSafeArrays(upper), oper));
}

std::string StmtContext::exprToStringWithSafeArrays(Expr *expr) {
  std::string initialStr = Utils::stmtToString(expr);
  std::vector<ArraySubscriptExpr *> rawAccesses;
  Utils::getExprArrayAccesses(expr, rawAccesses);
  for (const auto &access: rawAccesses) {
    auto accesses = DataAccessHandler::buildDataAccesses(access, true);
    std::string accessStr = accesses.back().toString(accesses);
    initialStr = iegenlib::replaceInString(
        initialStr, Utils::stmtToString(access), accessStr);
  }
  return initialStr;
}

std::string StmtContext::getItersTupleString() {
  std::ostringstream os;
  os << "[";
  if (iterators.empty()) {
    os << "0";
  } else {
    for (const auto &it: iterators) {
      if (it != *iterators.begin()) {
        os << ",";
      }
      os << it;
    }
  }
  os << "]";
  return os.str();
}

}  // namespace spf_ie
