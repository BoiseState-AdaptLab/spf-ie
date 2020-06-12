#include <iostream>

#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"

using namespace clang;

namespace pdfg_c {

class Builder {
   public:
    explicit Builder() : stmtNumber(0){};
    void processFunction(FunctionDecl* funcDecl) {
        CompoundStmt* funcBody = cast<CompoundStmt>(funcDecl->getBody());
        for (CompoundStmt::const_body_iterator it = funcBody->body_begin();
             it != funcBody->body_end(); ++it) {
            llvm::errs() << "S" << stmtNumber++ << ": " << *it << "\n";
        }
    }

   private:
    int stmtNumber;
};
}  // namespace pdfg_c

