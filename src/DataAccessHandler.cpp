#include "DataAccessHandler.hpp"

#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "Driver.hpp"
#include "Utils.hpp"
#include "clang/AST/Expr.h"

using namespace clang;

namespace spf_ie {

/* ArrayAccess */

std::string ArrayAccess::toString(const std::vector<ArrayAccess> &potentialSubaccesses) const {
  std::ostringstream os;
  os << DATA_SPACE_DELIMITER << this->arrayName << DATA_SPACE_DELIMITER;
  os << "(";
  bool first = true;
  for (const auto &it: this->indexes) {
    if (!first) {
      os << ",";
    } else {
      first = false;
    }
    std::string indexString;
    if (auto *asArrayAccess = dyn_cast<ArraySubscriptExpr>(it->IgnoreParenImpCasts())) {
      // there is another array access used as an index for this one
      bool foundSubaccess = false;
      for (const auto &it: potentialSubaccesses) {
        if (it.sourceId == asArrayAccess->getID(*Context)) {
          foundSubaccess = true;
          indexString = it.toString(potentialSubaccesses);
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
    os <<
       indexString;
  }
  os << ")";
  return os.
      str();
}

/* DataAccessHandler */

void DataAccessHandler::processAsReads(Expr *expr) {
  std::vector<ArraySubscriptExpr *> reads;
  Utils::getExprArrayAccesses(expr, reads);
  for (const auto &read: reads) {
    processAsDataAccess(read, true);
  }
}

void DataAccessHandler::processAsWrite(ArraySubscriptExpr *expr) {
  processAsDataAccess(expr, false);
}

void DataAccessHandler::processAsDataAccess(ArraySubscriptExpr *fullExpr,
                                            bool isRead) {
  auto accesses = buildDataAccesses(fullExpr, isRead);

  for (const auto &access: accesses) {
    dataSpaces.emplace(access.arrayName);
    arrayAccesses.push_back(access);
  }
}

std::vector<ArrayAccess> DataAccessHandler::buildDataAccesses(
    ArraySubscriptExpr *fullExpr, bool isRead) {
  std::vector<ArrayAccess> accesses;
  doBuildDataAccessesWork(fullExpr, isRead, accesses);
  return accesses;
}

int DataAccessHandler::getArrayExprInfo(ArraySubscriptExpr *fullExpr,
                                        std::stack<Expr *> *currentInfo) {
  if (currentInfo->size() >= MAX_ARRAY_DIM) {
    return false;
  }
  currentInfo->push(fullExpr->getIdx()->IgnoreParenImpCasts());
  Expr *baseExpr = fullExpr->getBase()->IgnoreParenImpCasts();
  if (auto *baseArrayAccess =
      dyn_cast<ArraySubscriptExpr>(baseExpr)) {
    return getArrayExprInfo(baseArrayAccess, currentInfo);
  } else {
    currentInfo->push(baseExpr);
    return true;
  }
}
void DataAccessHandler::doBuildDataAccessesWork(ArraySubscriptExpr *fullExpr,
                                                bool isRead,
                                                std::vector<ArrayAccess> &existingAccesses) {
  // extract information from subscript expression
  std::stack<Expr *> info;
  if (!getArrayExprInfo(fullExpr, &info)) {
    Utils::printErrorAndExit("Array dimension exceeds maximum of " +
                                 std::to_string(MAX_ARRAY_DIM),
                             fullExpr);
  }

  // construct ArrayAccess object
  Expr *baseAccess = info.top();
  info.pop();
  std::vector<Expr *> indexes;
  while (!info.empty()) {
    // recurse when an index is itself another array access; such
    // sub-accesses are always reads
    if (auto *indexAsArrayAccess =
        dyn_cast<ArraySubscriptExpr>(info.top())) {
      doBuildDataAccessesWork(indexAsArrayAccess, true, existingAccesses);
    }
    indexes.push_back(info.top());
    info.pop();
  }

  existingAccesses
      .emplace_back(ArrayAccess(Utils::stmtToString(baseAccess), fullExpr->getID(*Context), indexes, isRead));
}

}  // namespace spf_ie
