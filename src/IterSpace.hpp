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

// contains a statement and associated information (iteration space and
// execution schedules)
struct IterSpace {
    IterSpace() {}
    IterSpace(IterSpace* other) {
        iterators = other->iterators;
        constraints = other->constraints;
    }

    // variables being iterated over
    std::vector<ValueDecl*> iterators;
    // constraints on iteration (inequalities and equalities)
    std::vector<std::shared_ptr<std::tuple<Expr*, Expr*, BinaryOperatorKind>>>
        constraints;

    std::string toString(ASTContext* Context) {
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
                    os << " and ";
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

    // enter* and exit* methods add iterators and constraints when entering a
    // new scope, remove when leaving the scope

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
    // convenience function to add a new constraint from the given parameters
    void makeAndInsertConstraint(Expr* lower, Expr* upper,
                                 BinaryOperatorKind oper) {
        constraints.push_back(
            std::make_shared<std::tuple<Expr*, Expr*, BinaryOperatorKind>>(
                lower, upper, oper));
    }
};
}  // namespace pdfg_c
