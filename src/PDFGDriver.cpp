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
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::tooling;

namespace pdfg_c {

ASTContext *Context;

class PDFGConsumer : public ASTConsumer {
   public:
    explicit PDFGConsumer(std::string fileName) : fileName(fileName) {}
    virtual void HandleTranslationUnit(ASTContext &Context) {
        llvm::errs() << "\nProcessing: " << fileName << "\n";
        llvm::errs() << "=================================================\n\n";
        TranslationUnitDecl *transUnitDecl = Context.getTranslationUnitDecl();
        for (auto it : transUnitDecl->decls()) {
            FunctionDecl *func = dyn_cast<FunctionDecl>(it);
            if (func && func->doesThisDeclarationHaveABody()) {
                std::unique_ptr<PDFGLFuncBuilder> builder =
                    std::make_unique<PDFGLFuncBuilder>(&Context);
                builder->processFunction(func);
                builder->printInfo();
                builder->addIEGenSets();
                builder->genIEGenSets(fileName);
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
        // initializing globally-accessible ASTContext
        Context = &Compiler.getASTContext();
        return std::unique_ptr<ASTConsumer>(new PDFGConsumer(InFile.str()));
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
