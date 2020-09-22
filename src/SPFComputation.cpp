#include "SPFComputation.hpp"

#include <vector>

#include "Utils.hpp"
#include "clang/AST/Expr.h"
#include "llvm/Support/raw_ostream.h"

namespace spf_ie {

/* SPFComputation */

SPFComputation::printInfo() {
    // TODO: implement
    llvm::outs() << "implement me!";
}

/* IEGenStmtInfo */

void IEGenStmtInfo::processReads(Expr* expr) {
    Expr* usableExpr = expr->IgnoreParenImpCasts();
    if (BinaryOperator* binOper = dyn_cast<BinaryOperator>(usableExpr)) {
        processReads(binOper->getLHS());
        processReads(binOper->getRHS());
    } else if (ArraySubscriptExpr* asArrayAccessExpr =
                   dyn_cast<ArraySubscriptExpr>(usableExpr)) {
        dataReads.push_back(ArrayAccess::makeArrayAccess(asArrayAccessExpr));
    }
}

void IEGenStmtInfo::processWrite(ArraySubscriptExpr* expr) {
    dataWrites.push_back(ArrayAccess::makeArrayAccess(expr));
}

}  // namespace spf_ie
