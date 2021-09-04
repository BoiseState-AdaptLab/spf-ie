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

#include "SPFComputationBuilder.hpp"
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

static llvm::cl::opt<bool> PrintOutputToConsole(
	"print-info", llvm::cl::desc("Output info to console"));

namespace spf_ie {

const ASTContext *Context;

class SPFConsumer : public ASTConsumer {
public:
  explicit SPFConsumer(llvm::StringRef fileName) : fileName(fileName.str()) {}
  void HandleTranslationUnit(ASTContext &Ctx) override {
	// initializing globally-accessible ASTContext
	Context = &Ctx;
	llvm::errs() << "\nProcessing: " << fileName << "\n";
	if (PrintOutputToConsole) {
	  llvm::errs()
		  << "=================================================\n\n";
	}
	SPFComputationBuilder builder;
	// process each function (with a body) in the file
	bool builtAComputation = false;
	for (auto it: Context->getTranslationUnitDecl()->decls()) {
	  auto *func = dyn_cast<FunctionDecl>(it);
	  if (func && func->doesThisDeclarationHaveABody()) {
		if (PrintOutputToConsole) {
		  llvm::outs()
			  << "FUNCTION: " << func->getQualifiedNameAsString()
			  << "\n";
		  Utils::printSmallLine();
		  llvm::outs() << "\n";
		}
		std::unique_ptr<iegenlib::Computation> computation =
			builder.buildComputationFromFunction(func);
		builtAComputation = true;
		if (PrintOutputToConsole) {
		  computation->printInfo();
		}
	  }
	}
	if (!builtAComputation) {
	  llvm::errs() << "No valid functions found for processing!\n";
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
  PrintOutputToConsole.addCategory(SPFToolCategory);
  CommonOptionsParser OptionsParser(argc, argv, SPFToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
				 OptionsParser.getSourcePathList());

  return Tool.run(newFrontendActionFactory<SPFFrontendAction>().get());
}
