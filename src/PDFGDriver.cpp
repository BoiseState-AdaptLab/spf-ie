#include <iostream>

#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/StringRef.h"

using namespace clang;
using namespace clang::tooling;

namespace pdfg_c {
class PDFGConsumer : public ASTConsumer {
   public:
    explicit PDFGConsumer(ASTContext *Context, std::string fileName)
        : fileName(fileName) {}
    // defines the action to perform per translation unit
    virtual void HandleTranslationUnit(ASTContext &Context) {
        llvm::errs() << fileName << "\n";
    }

   private:
    std::string fileName;
};

class PDFGFrontendAction : public ASTFrontendAction {
   public:
    // called to create a consumer for each translation unit
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::unique_ptr<ASTConsumer>(
            new PDFGConsumer(&Compiler.getASTContext(), InFile.str()));
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
