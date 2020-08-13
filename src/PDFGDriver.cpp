#include "PDFGDriver.hpp"

#include <memory>

#include "PDFGLFuncBuilder.hpp"
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

namespace pdfg_c {

ASTContext *Context;

class PDFGConsumer : public ASTConsumer {
   public:
    explicit PDFGConsumer(llvm::StringRef fileName)
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
                std::unique_ptr<PDFGLFuncBuilder> builder =
                    std::make_unique<PDFGLFuncBuilder>();
                builder->processFunction(func);
                builder->printInfo();
            }
        }
    }

   private:
    std::string fileName;
};

class PDFGFrontendAction : public ASTFrontendAction {
   public:
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::unique_ptr<ASTConsumer>(new PDFGConsumer(InFile));
    }
};

}  // namespace pdfg_c

using namespace pdfg_c;

static llvm::cl::OptionCategory PDFGToolCategory("pdfg-c options");

int main(int argc, const char **argv) {
    CommonOptionsParser OptionsParser(argc, argv, PDFGToolCategory);
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    return Tool.run(newFrontendActionFactory<PDFGFrontendAction>().get());
}
