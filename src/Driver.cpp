/*!
 * \file Driver.cpp
 *
 * \brief Driver for the program, processing command-line input.
 *
 * Contains the basic setup for a Clang tool and the entry point of the
 * program.
 *
 * \author Anna Rift
 */

#include "Driver.hpp"

#include <memory>

#include "SPFFuncBuilder.hpp"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::tooling;

namespace spf_ie {

ASTContext *Context;

class SPFConsumer : public ASTConsumer {
   public:
    explicit SPFConsumer(llvm::StringRef fileName)
        : fileName(fileName.str()) {}
    virtual void HandleTranslationUnit(ASTContext &Ctx) {
        // initializing globally-accessible ASTContext
        Context = &Ctx;
        llvm::errs() << "\nProcessing: " << fileName << "\n";
        llvm::errs() << "=================================================\n\n";
        // process each function (with a body) in the file
        for (auto it : Context->getTranslationUnitDecl()->decls()) {
            FunctionDecl *func = dyn_cast<FunctionDecl>(it);
            if (func && func->doesThisDeclarationHaveABody()) {
                std::unique_ptr<SPFFuncBuilder> builder =
                    std::make_unique<SPFFuncBuilder>();
                builder->processFunction(func);
                builder->printInfo();
            }
        }
    }

   private:
    std::string fileName;
};

class SPFFrontendAction : public ASTFrontendAction {
   public:
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::unique_ptr<ASTConsumer>(new SPFConsumer(InFile));
    }
};

}  // namespace spf_ie

using namespace spf_ie;

static llvm::cl::OptionCategory SPFToolCategory("spf-ie options");

//! Instantiate and run the Clang tool
int main(int argc, const char **argv) {
    CommonOptionsParser OptionsParser(argc, argv, SPFToolCategory);
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    return Tool.run(newFrontendActionFactory<SPFFrontendAction>().get());
}
