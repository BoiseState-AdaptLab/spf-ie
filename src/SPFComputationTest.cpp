#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Driver.hpp"
#include "SPFComputation.hpp"
#include "SPFComputationBuilder.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Serialization/PCHContainerOperations.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"
#include "iegenlib.h"
#include "llvm/ADT/StringRef.h"

using namespace clang;

namespace spf_ie {

const ASTContext *Context;

}

using namespace spf_ie;

class SPFComputationTest : public ::testing::Test {
   protected:
    virtual void SetUp() override {}
    virtual void TearDown() override {}
    std::vector<SPFComputation> buildSPFComputationsFromCode(std::string code) {
        std::unique_ptr<ASTUnit> AST = tooling::buildASTFromCode(
            code, "input.cpp", std::make_shared<PCHContainerOperations>());
        Context = &AST->getASTContext();

        std::vector<SPFComputation> computations;
        SPFComputationBuilder builder;
        for (auto it : Context->getTranslationUnitDecl()->decls()) {
            FunctionDecl *func = dyn_cast<FunctionDecl>(it);
            if (func && func->doesThisDeclarationHaveABody()) {
                computations.push_back(
                    builder.buildComputationFromFunction(func));
            }
        }
        return computations;
    }
};

TEST_F(SPFComputationTest, matrix_add) {
    std::string code =
        "void matrix_add(int a, int b, int x[a][b], int y[a][b], int sum[a][b]) { \
    int i; \
    int j; \
    for (i = 0; i < a; i++) { \
        for (j = 0; j < b; j++) { \
            sum[i][j] = x[i][j] + y[i][j]; \
        } \
    } \
}";
    std::vector<SPFComputation> computations =
        buildSPFComputationsFromCode(code);
    ASSERT_EQ(computations.size(), 1);
    SPFComputation computation = computations.back();

    EXPECT_EQ(computation.dataSpaces,
              std::unordered_set<std::string>({"sum", "x", "y"}));
    ASSERT_EQ(computation.stmtsInfoMap.size(), 3);
}

// set up and run tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
