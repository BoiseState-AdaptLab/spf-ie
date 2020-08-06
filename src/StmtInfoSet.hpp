#include <memory>
#include <tuple>
#include <vector>
#include <sstream>

#include "Utils.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/StringRef.h"

using namespace clang;

namespace pdfg_c {

// Represents an if statement as a list of constraints
// Don't confuse it with clang's IfStmt
struct IfStmnt {
    std::vector<std::string> constraints;
    ASTContext* Context;

    IfStmnt (ASTContext* Context) : Context(Context) {}
    IfStmnt (ASTContext* Context, IfStmt* stmt) : Context(Context) {
        if (BinaryOperator* b = cast<BinaryOperator>(stmt->getCond())) {
            parseBinOp(b);
        }
    }

    // Print this struct
    void print() {
        for (auto it = constraints.begin(); it != constraints.end(); ++it) {
            if (it != constraints.begin()) {llvm::outs() << " && ";}
            llvm::outs() << (*it);
        }
        llvm::outs() << "\n";
    }

    // Parse a binary operation into constraints
    void parseBinOp(BinaryOperator* b) {
        // TODO: Handle OR statements (e.g. i >= 5 || i <= -5)
        // Check if either side of the binary operator is itself a binary operator
        bool lh = isa<BinaryOperator>(b->getLHS());
        bool rh = isa<BinaryOperator>(b->getRHS());
        if (lh && rh) {
            parseBinOp(cast<BinaryOperator>(b->getLHS()));
            parseBinOp(cast<BinaryOperator>(b->getRHS()));
        // If both sides are simple expressions, add as a constraint
        } else {
            constraints.push_back(constraintString(b->getLHS(), b->getRHS(),
                                                   b->getOpcode()));
       }
    }

    // Turns two Expr objects and a binary operation code into a constraint string
    std::string constraintString(Expr* lhs, Expr* rhs, BinaryOperatorKind oper) {
        std::string output;
        llvm::raw_string_ostream os(output);
        os << Utils::exprToString(lhs, Context) << " "
            << Utils::binaryOperatorKindToString(oper, Context)
            << " " << Utils::exprToString(rhs, Context);
        return os.str();
    }
};

// Represents a for loop as an iteration variable and a list of constraints
// Don't confuse it with clang's ForStmt
struct ForStmnt : public IfStmnt {
    std::string iter;

    ForStmnt (ASTContext* Context) : IfStmnt(Context) {}
    ForStmnt (ASTContext* Context, ForStmt* stmt) : IfStmnt(Context) {
        parseFor(stmt);
    }

    // Print this struct
    void print() {
        llvm::outs() << "[" << iter << "]: ";
        IfStmnt::print();
    }

    // Parse ForStmt
    void parseFor(ForStmt* stmt) {
        // Set to true if we can parse the increment statement
        bool canParse = false;
        // Stores whether the loop is incrementing or decrementing
        bool increment;
        // Get increment expression
        Expr* incExpr = stmt->getInc();
        if (isa<UnaryOperator>(incExpr)){
            UnaryOperator* inc = cast<UnaryOperator>(incExpr);
            if (inc->isIncrementDecrementOp()) {
                // Get the increment operator
                UnaryOperatorKind oper = inc->getOpcode();
                // Check if it is a positive operator (++i or i++)
                increment = (oper == UO_PreInc || oper == UO_PostInc);
                canParse = true;
            }
        } else if (isa<BinaryOperator>(incExpr)) {
            BinaryOperator* inc = cast<BinaryOperator>(incExpr);
            // Get increment operator
            BinaryOperatorKind oper = inc->getOpcode();
            // Operator is '+=' or '-='
            if (oper == BO_AddAssign || oper == BO_SubAssign) {
                //TODO: What do we do with the integer literal increment?
                // Saves increment value
                Expr::EvalResult result;
                // Rhs must be an integer != 0
                if (inc->getRHS()->EvaluateAsInt(result, *Context)) {
                    llvm::APSInt incVal = result.Val.getInt();
                    if (incVal != 0) {
                        // Check if it is a positive operator (+=)                  
                        increment = (oper == BO_AddAssign) == incVal.isNonNegative();
                        canParse = true;
                    }
                }
            // Operator is '='
            } else if (oper == BO_Assign && isa<BinaryOperator>(inc->getRHS())) {
                // This is the rhs of the increment statement
                // (e.g. with i = i + 1, this is i + 1)
                BinaryOperator* binOp = cast<BinaryOperator>(inc->getRHS());
                // Get the binary operation that conrols loop incrementing
                BinaryOperatorKind oper = binOp->getOpcode();
                // Get variable being incremented
                std::string iterStr = Utils::exprToString(inc->getLHS(),
                                                          Context).str();
                // Must be addition or subtraction
                if (binOp->isAdditiveOp()) {
                    // Get our lh and rh expression, also in string form
                    Expr* lhs = binOp->getLHS();
                    Expr* rhs = binOp->getRHS();
                    std::string lhStr = Utils::exprToString(lhs, Context).str();
                    std::string rhStr = Utils::exprToString(rhs, Context).str();
                    // Saves increment value
                    Expr::EvalResult result;
                    // Left side is iter var, right side is integer (e.g. i + 1)
                    if ((lhStr.compare(iterStr) == 0 &&
                         rhs->EvaluateAsInt(result, *Context)) ||
                    // Right side is iter var, left side is integer (e.g. 1 + i)
                    // Operation can't be '-' (e.g. 1 - i)
                        (rhStr.compare(iterStr) == 0 && oper != BO_Sub &&
                         lhs->EvaluateAsInt(result, *Context))){

                        llvm::APSInt incVal = result.Val.getInt();
                        if (incVal != 0) {
                            // Calculate if our increment is positive or negative
                            increment = (oper == BO_Add) == incVal.isNonNegative();
                            canParse = true;
                        }
                    }
                }
            }
        }
        if (!canParse) {
            llvm::outs() << "Cannot parse loop increment statement ";
            incExpr->dumpPretty(*Context);
            llvm::outs() << "\n";
            return;
        }

        // Parse initializer expression
        Stmt* initStmt = stmt->getInit();
        if (isa<BinaryOperator>(initStmt)) {
            parseInit(cast<BinaryOperator>(initStmt), increment);
        } else if (isa<DeclStmt>(initStmt)) {
            parseInit(cast<DeclStmt>(initStmt), increment);
        } else {
            return;
        }
 
        // Parse conditions
        if (isa<BinaryOperator>(stmt->getCond())) {            
            parseBinOp(cast<BinaryOperator>(stmt->getCond()));
        }
    }

  private:
    // Parse the init statement if it is a binary operator
    void parseInit(BinaryOperator* init, bool increment) {
        iter = Utils::exprToString(init->getLHS(), Context).str();
        // If we are incrementing, then iter >= lb otherwise iter <= ub
        BinaryOperatorKind oper = (increment ? BO_GE : BO_LE);
        constraints.push_back(constraintString(init->getLHS(), init->getRHS(), oper));
    }

    // Parse the init statement if it is a declaration
    void parseInit(DeclStmt* init, bool increment) {
        if (VarDecl* initVar = cast<VarDecl>(init->getSingleDecl())) {
            iter = initVar->getNameAsString();
            // If we are incrementing, then iter >= lb otherwise iter <= ub
            BinaryOperatorKind oper = (increment ? BO_GE : BO_LE);
            std::string output;
            llvm::raw_string_ostream os(output);
            os << iter << " "
               << Utils::binaryOperatorKindToString(oper, Context)
               << " " << Utils::exprToString(initVar->getInit(), Context);
            constraints.push_back(os.str());
        }
    }
};

// one entry in an execution schedule, which may be a variable or a number
struct ScheduleVal {
    ScheduleVal(std::string var) : var(var), valueIsVar(true) {}
    ScheduleVal(int num) : num(num), valueIsVar(false) {}

    std::string var;
    int num;
    bool valueIsVar;
};

// contains associated information for a statement (iteration space and
// execution schedules)
struct StmtInfoSet {
    StmtInfoSet() {}
    StmtInfoSet(StmtInfoSet* other) {
        ifStmts = other->ifStmts;
        forStmts = other->forStmts;
        schedule = other->schedule;
    }

    std::string getIterSpaceString() {
        std::string output, output2, output3;
        llvm::raw_string_ostream os(output);
        llvm::raw_string_ostream iters(output2), constrs(output3);
        iters << "[";
        // Write all for loops
        for (auto it = forStmts.begin(); it != forStmts.end(); ++it) {
            if (it != forStmts.begin()) {
                iters << ",";
                constrs << " && ";
            }
            iters << (*it)->iter;
            std::vector<std::string>* vec = &((*it)->constraints);
            for (auto str = vec->begin(); str != vec->end(); ++str) {
                if (str != vec->begin()) {
                    constrs << " && ";
                }
                constrs << (*str);
            }
        }
        iters << "]";
        // Write all if statements
        for (auto it = ifStmts.begin(); it != ifStmts.end(); ++it) {
            if (it != ifStmts.begin()) {
                constrs << " && ";
            }
            std::vector<std::string>* vec = &((*it)->constraints);
            for (auto str = vec->begin(); str != vec->end(); ++str) {
                if (str != vec->begin()) {
                    constrs << " && ";
                }   
                constrs << (*str);
            }
        }   
        os << "{" << iters.str() << ": " << constrs.str() << "}";
        return os.str();
    }

    std::string getExecScheduleString() {
        std::string output;
        llvm::raw_string_ostream os(output);
        os << "{[";
        // Write all iterator variables
        for (auto it = forStmts.begin(); it != forStmts.end(); ++it) {
            if (it != forStmts.begin()) {
                os << ",";
            }
            os << (*it)->iter;
        }
        os << "]->[";
        // Write schedule
        for (auto it = schedule.begin(); it != schedule.end(); ++it) {
            if (it != schedule.begin()) {
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

    // move statement number forward in the schedule
    void advanceSchedule() {
        if (schedule.empty() || schedule.back()->valueIsVar) {
            schedule.push_back(std::make_shared<ScheduleVal>(0));
        } else {
            int num = schedule.back()->num;
            schedule.pop_back();
            schedule.push_back(std::make_shared<ScheduleVal>(num + 1));
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

    void enterFor(ForStmt* forStmt, ASTContext* Context) {
        // Add for loop
        auto stmt = std::make_shared<ForStmnt>(Context, forStmt);
        forStmts.push_back(stmt);
        // Update schedule
        schedule.push_back(std::make_shared<ScheduleVal>(stmt->iter));
    }

    void exitFor() {
        forStmts.pop_back();
        // Remove place in for loop from schedule
        schedule.pop_back();
        // Remove for loop from schedule
        schedule.pop_back();
    }

    void enterIf(IfStmt* ifStmt, ASTContext* Context) {
        ifStmts.push_back(std::make_shared<IfStmnt>(Context, ifStmt));
    }

    void exitIf() { ifStmts.pop_back(); }

   private:
    // List of if statements
    std::vector<std::shared_ptr<IfStmnt>> ifStmts;
    // List of for loops
    std::vector<std::shared_ptr<ForStmnt>> forStmts;
    // execution schedule, which begins at 0
    std::vector<std::shared_ptr<ScheduleVal>> schedule; 
};
}  // namespace pdfg_c
