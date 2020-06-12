#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/StringRef.h"

using namespace clang;

namespace pdfg_c {

// represents the iteration space of a statement
struct IterSpace {
    IterSpace() {}
    IterSpace(IterSpace* other) {
        iterators = other->iterators;
        constraints = other->constraints;
    }

    std::vector<ValueDecl*> iterators;
    std::vector<std::shared_ptr<std::tuple<Expr*, Expr*, BinaryOperatorKind>>>
        constraints;

    void enterFor(ForStmt* forStmt) {
        BinaryOperator* init = cast<BinaryOperator>(forStmt->getInit());
        makeAndInsertConstraint(init->getLHS(), init->getRHS(),
                                BinaryOperatorKind::BO_LE);
        BinaryOperator* cond = cast<BinaryOperator>(forStmt->getCond());
        makeAndInsertConstraint(cond->getLHS(), cond->getRHS(),
                                cond->getOpcode());
        iterators.push_back(
            cast<DeclRefExpr>(init->getLHS()->IgnoreImplicit())->getDecl());
    }

    void exitFor() {
        constraints.pop_back();
        constraints.pop_back();
        iterators.pop_back();
    }

    void enterIf(IfStmt* ifStmt) {
        BinaryOperator* cond = cast<BinaryOperator>(ifStmt->getCond());
        makeAndInsertConstraint(cond->getLHS(), cond->getRHS(),
                                cond->getOpcode());
    }

    void exitIf() { constraints.pop_back(); }

   private:
    void makeAndInsertConstraint(Expr* lower, Expr* upper,
                                 BinaryOperatorKind oper) {
        constraints.push_back(
            std::make_shared<std::tuple<Expr*, Expr*, BinaryOperatorKind>>(
                lower, upper, oper));
    }
};
}  // namespace pdfg_c
