#ifndef PDFG_UTILS_HPP
#define PDFG_UTILS_HPP

#include <map>

#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/StringRef.h"

using namespace clang;

namespace pdfg_c {

class Utils {
   public:
    static void printErrorAndExit(llvm::StringRef message);

    static void printErrorAndExit(llvm::StringRef message, Stmt* stmt);

    // print a line (horizontal separator) to stdout
    static void printSmallLine();

    static std::string stmtToString(Stmt* stmt);

    static std::string exprToString(Expr* expr);

    static std::string binaryOperatorKindToString(BinaryOperatorKind bo);

   private:
    // valid operators for use in constraints
    static std::map<BinaryOperatorKind, llvm::StringRef> operatorStrings;

    Utils() = delete;
};

}  // namespace pdfg_c

#endif
