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

#include "ComputationBuilder.hpp"
#include "Utils.hpp"
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

using namespace clang;
using namespace clang::tooling;

static llvm::cl::opt<bool> FrontendOnly(
    "frontend-only", llvm::cl::desc("Just run the spf-ie frontend and output Computation IR to console"));

static llvm::cl::opt<std::string> EntryPoint(
    "entry-point", llvm::cl::desc(
        "Entry point for the spf-ie tool, only the specified "
        "function will be translated"));

namespace spf_ie {

const ASTContext *Context;

class SPFConsumer : public ASTConsumer {
public:
  explicit SPFConsumer(llvm::StringRef fileName) : fileName(fileName.str()) {}
  void HandleTranslationUnit(ASTContext &Ctx) override {
    if (EntryPoint.empty()) {
      llvm::errs() << "\033[31m--entry-point flag must be specified (-h for usage)\033[0m\n";
      exit(1);
    }
    // initializing globally-accessible ASTContext
    Context = &Ctx;
    llvm::errs() << "\nProcessing: " << fileName << "\n";
    llvm::errs()
        << "=================================================\n\n";
    ComputationBuilder builder;
    // locate and process the target function
    bool builtAComputation = false;
    for (auto it: Context->getTranslationUnitDecl()->decls()) {
      auto *func = dyn_cast<FunctionDecl>(it);
      if (func && func->doesThisDeclarationHaveABody() &&
          func->getQualifiedNameAsString() == EntryPoint) {
        iegenlib::Computation *computation =
            builder.buildComputationFromFunction(func);
        builtAComputation = true;
        if (FrontendOnly) {
          llvm::errs()
              << "Computation IR for function '" << func->getQualifiedNameAsString()
              << "'\n";
          llvm::errs() << "---------------\n\n";
          computation->printInfo();
        } else {
          llvm::errs() << "Codegen for function '" << func->getQualifiedNameAsString() << "':\n\n";
          computation->finalize();
          std::string codegen = computation->codeGen();
          llvm::outs() << codegen;
        }
        delete computation;
      }
    }
    if (!builtAComputation) {
      llvm::errs() << "Could not locate definition of the target function '" << EntryPoint << "'!\n";
      exit(1);
    }
  }

private:
  std::string fileName;
};

class SPFFrontendAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
      CompilerInstance &Compiler, llvm::StringRef InFile) override {
    return std::unique_ptr<ASTConsumer>(new SPFConsumer(InFile));
  }
};

}  // namespace spf_ie

using namespace spf_ie;

static llvm::cl::OptionCategory SPFToolCategory("spf-ie options");

//! Instantiate and run the Clang tool
int main(int argc, const char **argv) {
  FrontendOnly.addCategory(SPFToolCategory);
  EntryPoint.addCategory(SPFToolCategory);
  CommonOptionsParser OptionsParser(argc, argv, SPFToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  return Tool.run(newFrontendActionFactory<SPFFrontendAction>().get());
}
