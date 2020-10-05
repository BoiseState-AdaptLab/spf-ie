#include "DataAccessHandler.hpp"

#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "Driver.hpp"
#include "Utils.hpp"
#include "clang/AST/Expr.h"

using namespace clang;

namespace spf_ie {

/* DataAccessHandler */

void DataAccessHandler::processAsReads(Expr* expr) {
    Expr* usableExpr = expr->IgnoreParenImpCasts();
    if (BinaryOperator* binOper = dyn_cast<BinaryOperator>(usableExpr)) {
        processAsReads(binOper->getLHS());
        processAsReads(binOper->getRHS());
    } else if (ArraySubscriptExpr* asArrayAccessExpr =
                   dyn_cast<ArraySubscriptExpr>(usableExpr)) {
        addDataAccess(asArrayAccessExpr, true);
    }
}

void DataAccessHandler::processAsWrite(ArraySubscriptExpr* expr) {
    addDataAccess(expr, false);
}

void DataAccessHandler::addDataAccess(ArraySubscriptExpr* fullExpr,
                                      bool isRead) {
    // extract information from subscript expression
    std::stack<Expr*> info;
    if (getArrayExprInfo(fullExpr, &info)) {
        Utils::printErrorAndExit("Array dimension exceeds maximum of " +
                                     std::to_string(MAX_ARRAY_DIM),
                                 fullExpr);
    }

    // construct ArrayAccess object
    Expr* base = info.top();
    info.pop();
    std::vector<Expr*> indexes;
    while (!info.empty()) {
        // recurse when an index is itself another array access; such
        // sub-accesses are always reads
        if (ArraySubscriptExpr* indexAsArrayAccess =
                dyn_cast<ArraySubscriptExpr>(info.top())) {
            addDataAccess(indexAsArrayAccess, true);
        }
        indexes.push_back(info.top());
        info.pop();
    }
    dataSpaces.emplace(Utils::stmtToString(base));
    ArrayAccess access =
        ArrayAccess(fullExpr->getID(*Context), base, indexes, isRead);
    arrayAccesses.push_back({makeStringForArrayAccess(&access), access});
}

std::string DataAccessHandler::makeStringForArrayAccess(ArrayAccess* access) {
    std::ostringstream os;
    os << Utils::stmtToString(access->base);
    os << "(";
    bool first = true;
    for (const auto& it : access->indexes) {
        if (!first) {
            os << ",";
        } else {
            first = false;
        }
        std::string indexString;
        if (ArraySubscriptExpr* asArrayAccess =
                dyn_cast<ArraySubscriptExpr>(it->IgnoreParenImpCasts())) {
            // there is another array acccess used as an index for this one;
            // it should have already been processed (depth-first), so we look
            // for its string equivalent in thealready-processed accesses
            bool foundSubaccess = false;
            for (const auto& it : arrayAccesses) {
                if (it.second.id == asArrayAccess->getID(*Context)) {
                    foundSubaccess = true;
                    indexString = it.first;
                    break;
                }
            }
            if (!foundSubaccess) {
                Utils::printErrorAndExit(
                    "Could not stringify array access because its sub-access "
                    "had (printed below) not already been processed.\nThis "
                    "point should be unreachable -- this is a bug.",
                    asArrayAccess);
            }
        } else {
            indexString = Utils::stmtToString(it);
        }
        os << indexString;
    }
    os << ")";
    return os.str();
}

int DataAccessHandler::getArrayExprInfo(ArraySubscriptExpr* fullExpr,
                                        std::stack<Expr*>* currentInfo) {
    if (currentInfo->size() >= MAX_ARRAY_DIM) {
        return 1;
    }
    currentInfo->push(fullExpr->getIdx()->IgnoreParenImpCasts());
    Expr* baseExpr = fullExpr->getBase()->IgnoreParenImpCasts();
    if (ArraySubscriptExpr* baseArrayAccess =
            dyn_cast<ArraySubscriptExpr>(baseExpr)) {
        return getArrayExprInfo(baseArrayAccess, currentInfo);
    } else {
        currentInfo->push(baseExpr);
        return 0;
    }
}

}  // namespace spf_ie
