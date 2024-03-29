#include "DataAccessHandler.hpp"

#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "Driver.hpp"
#include "Utils.hpp"
#include "ComputationBuilder.hpp"
#include "clang/AST/Expr.h"

using namespace clang;

namespace spf_ie {

/* DataAccess */

std::string DataAccess::toString(const std::vector<DataAccess> &potentialSubaccesses) const {
  std::ostringstream os;
  os << this->name;
  if (this->isArrayAccess) {
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
  }
  return os.str();
}

/* DataAccessHandler */

void DataAccessHandler::processExprAsRead(Expr *expr) {
  processSingleAccessExpr(expr, true);
}

void DataAccessHandler::processExprAsWrite(Expr *expr) {
  processSingleAccessExpr(expr, false);
}

void DataAccessHandler::processReadToScalarName(const std::string &name) {
  processAccessToScalarName(name, true);
}

void DataAccessHandler::processWriteToScalarName(const std::string &name) {
  processAccessToScalarName(name, false);
}

void DataAccessHandler::processAccessToScalarName(const std::string &name, bool isRead) {
  if (ComputationBuilder::positionContext->isIteratorName(name)) {
    return;
  }

  dataSpacesAccessed.emplace(name);
  stmtDataAccesses.push_back(DataAccess(name, 0, isRead, false, {}));
}

void DataAccessHandler::processSingleAccessExpr(Expr *fullExpr,
                                                bool isRead) {
  auto accesses = makeDataAccessesFromExpr(fullExpr, isRead);

  for (const auto &access: accesses) {
    // skip counting iterators as data accesses
    if (ComputationBuilder::positionContext->isIteratorName(access.name)) {
      continue;
    }
    dataSpacesAccessed.emplace(access.name);
    stmtDataAccesses.push_back(access);
  }
}

std::vector<DataAccess> DataAccessHandler::makeDataAccessesFromExpr(
    Expr *fullExpr, bool isRead) {
  std::vector<DataAccess> accesses;
  if (auto *asArraySubscriptExpr =
      dyn_cast<ArraySubscriptExpr>(fullExpr)) {
    doBuildArrayAccessWork(asArraySubscriptExpr, isRead, accesses);
  } else if (auto *asDeclRefExpr = dyn_cast<DeclRefExpr>(fullExpr)) {
    std::string varName = Utils::stmtToString(asDeclRefExpr);
    if (!ComputationBuilder::positionContext->isIteratorName(varName)) {
      accesses.emplace_back(
          DataAccess(varName, asDeclRefExpr->getID(*Context), isRead, false, {}));
    }
  }
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

void DataAccessHandler::doBuildArrayAccessWork(ArraySubscriptExpr *fullExpr,
                                               bool isRead,
                                               std::vector<DataAccess> &existingAccesses) {
  // extract information from subscript expression
  std::stack<Expr *> info;
  if (!getArrayExprInfo(fullExpr, &info)) {
    Utils::printErrorAndExit("Array dimension exceeds maximum of " +
                                 std::to_string(MAX_ARRAY_DIM),
                             fullExpr);
  }

  // construct DataAccess object
  Expr *baseAccess = info.top();
  info.pop();
  std::vector<Expr *> indexes;
  while (!info.empty()) {
    // recurse when an index is itself another array access; such
    // sub-accesses are always reads
    if (auto *indexAsArrayAccess =
        dyn_cast<ArraySubscriptExpr>(info.top())) {
      doBuildArrayAccessWork(indexAsArrayAccess, true, existingAccesses);
    }
    indexes.push_back(info.top());
    info.pop();
  }

  existingAccesses
      .emplace_back(DataAccess(Utils::stmtToString(baseAccess), fullExpr->getID(*Context), isRead, true, indexes));
}

}  // namespace spf_ie
