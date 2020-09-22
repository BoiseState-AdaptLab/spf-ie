#include "ArrayAccess.hpp"

#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "Utils.hpp"
#include "clang/AST/Expr.h"

using namespace clang;

namespace spf_ie {

/* ArrayAccess */

ArrayAccess ArrayAccess::makeArrayAccess(ArraySubscriptExpr* fullExpr) {
    std::stack<Expr*> info;
    if (getArrayExprInfo(fullExpr, &info)) {
        Utils::printErrorAndExit("Array dimension exceeds maximum of " +
                                     std::to_string(MAX_ARRAY_DIM),
                                 fullExpr);
    }
    Expr* base = info.top();
    info.pop();
    std::vector<Expr*> indexes;
    while (!info.empty()) {
        indexes.push_back(info.top());
        info.pop();
    }
    return ArrayAccess(base, indexes);
}

int ArrayAccess::getArrayExprInfo(ArraySubscriptExpr* fullExpr,
                                  std::stack<Expr*>* currentInfo) {
    if (currentInfo->size() >= MAX_ARRAY_DIM) {
        return 1;
    }
    currentInfo->push(fullExpr->getIdx());
    Expr* baseExpr = fullExpr->getBase()->IgnoreParenImpCasts();
    if (ArraySubscriptExpr* baseArrayAccess =
            dyn_cast<ArraySubscriptExpr>(baseExpr)) {
        return getArrayExprInfo(baseArrayAccess, currentInfo);
    }
    currentInfo->push(baseExpr);
    return 0;
}

std::string ArrayAccess::toString() {
    std::ostringstream os;
    os << Utils::stmtToString(base);
    os << "(";
    bool first = true;
    for (auto it = indexes.begin(); it != indexes.end(); ++it) {
        if (!first) {
            os << ",";
        } else {
            first = false;
        }
        std::string indexString;
        if (ArraySubscriptExpr* asArrayAccess =
                dyn_cast<ArraySubscriptExpr>((*it)->IgnoreParenImpCasts())) {
            // TODO: improve this inefficient solution
            indexString = makeArrayAccess(asArrayAccess).toString();
        } else {
            indexString = Utils::stmtToString(*it);
        }
        os << indexString;
    }
    os << ")";
    return os.str();
}

}  // namespace spf_ie
