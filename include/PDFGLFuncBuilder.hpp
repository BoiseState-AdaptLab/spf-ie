#ifndef PDFG_PDFGLFUNCBUILDER_HPP
#define PDFG_PDFGLFUNCBUILDER_HPP

#include <string>
#include <vector>

#include "StmtInfoSet.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"

using namespace clang;

namespace pdfg_c {

class PDFGLFuncBuilder {
   public:
    explicit PDFGLFuncBuilder();
    // entry point for each function; gather information about its statements
    void processFunction(FunctionDecl* funcDecl);

    // print collected information to stdout
    void printInfo();

   private:
    std::string functionName;
    std::vector<Stmt*> stmts;
    std::vector<StmtInfoSet> stmtInfoSets;
    StmtInfoSet currentStmtInfoSet;
    int largestScheduleDimension;

    // process the body of a control structure
    void processBody(Stmt* stmt);

    // process one statement, recursing if compound structure
    void processSingleStmt(Stmt* stmt);

    // add info about a stmt to the builder
    void addStmt(Stmt* stmt);
};

}  // namespace pdfg_c

#endif
