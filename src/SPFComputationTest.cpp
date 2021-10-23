/*!
 * \file SPFComputationTest.cpp
 *
 * \brief Regression tests which compare built SPFComputations to
 * expected values.
 *
 * \author Anna Rift
 */
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Driver.hpp"
#include "SPFComputationBuilder.hpp"
#include "Utils.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Serialization/PCHContainerOperations.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"
#include "iegenlib.h"

using namespace clang;
using namespace spf_ie;

const ASTContext *spf_ie::Context;

/*!
 * \class SPFComputationTest
 *
 * \brief Google Test fixture for SPFComputationBuilder.
 *
 * Contains tools for efficiently regression testing the output of the
 * SPFComputationBuilder against expected values for a given piece of code.
 */
class SPFComputationTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    iegenlib::Computation::resetNumRenamesCounters();
  }
  virtual void TearDown() override {}

  std::string replacementVarName = REPLACEMENT_VAR_BASE_NAME;

  //! Build a Computation from the first function in the provided code string.
  static iegenlib::Computation *
  buildComputationFromCode(const std::string &code) {
    std::unique_ptr<ASTUnit> AST = tooling::buildASTFromCode(
        code, "test_input.cpp", std::make_shared<PCHContainerOperations>());
    Context = &AST->getASTContext();

    SPFComputationBuilder builder;
    Computation *comp;
    bool builtComputation = false;
    for (auto it: Context->getTranslationUnitDecl()->decls()) {
      auto *func = dyn_cast<FunctionDecl>(it);
      if (func && func->doesThisDeclarationHaveABody()) {
        if (builtComputation) {
          Utils::printErrorAndExit("Multiple computations found in provided code snippet:\n" + code);
        }
        comp = builder.buildComputationFromFunction(func);
        builtComputation = true;
      }
    }

    if (!builtComputation) {
      Utils::printErrorAndExit("No Computation could be generated from the following provided code:\n" + code);
    }
    return comp;
  }

  //! EXPECT with gTest that two Computations are equal, component by component.
  void expectComputationsEqual(const Computation *actual, const Computation *expected) {
    ASSERT_EQ(expected->getName(), actual->getName());
    SCOPED_TRACE("Computation '" + actual->getName() + "'");

    ASSERT_EQ(expected->isComplete(), actual->isComplete());

    const auto expectedTransformations = expected->getTransformations();
    const auto actualTransformations = actual->getTransformations();
    ASSERT_EQ(expectedTransformations.size(), actualTransformations.size());
    for (unsigned int i = 0; i < expectedTransformations.size(); ++i) {
      EXPECT_EQ(*expectedTransformations[i], *actualTransformations[i]);
    }

    ASSERT_EQ(expected->getNumStmts(), actual->getNumStmts());
    for (unsigned int i = 0; i < actual->getNumStmts(); ++i) {
      SCOPED_TRACE("Statement " + std::to_string(i));
      expectStmtsEqual(actual->getStmt(i), expected->getStmt(i));
    }

    EXPECT_EQ(expected->getDataSpaces(), actual->getDataSpaces());

    ASSERT_EQ(expected->getNumParams(), actual->getNumParams());
    for (unsigned int i = 0; i < actual->getNumParams(); ++i) {
      SCOPED_TRACE("Parameter " + std::to_string(i));
      EXPECT_EQ(expected->getParameterName(i), actual->getParameterName(i));
      EXPECT_EQ(expected->getParameterType(i), actual->getParameterType(i));
    }

    EXPECT_EQ(expected->getReturnValues(), actual->getReturnValues());
    EXPECT_EQ(expected->getActiveOutValues(), actual->getActiveOutValues());

    EXPECT_TRUE(*actual == *expected);
  }

  //! EXPECT with gTest that two Stmts are equal, component by component.
  void expectStmtsEqual(const iegenlib::Stmt *actual, const iegenlib::Stmt *expected) {
    ASSERT_EQ(expected->isComplete(), actual->isComplete());
    ASSERT_EQ(expected->isDelimited(), actual->isDelimited());

    EXPECT_EQ(expected->getStmtSourceCode(), actual->getStmtSourceCode());

    EXPECT_EQ(expected->getIterationSpace()->prettyPrintString(),
              actual->getIterationSpace()->prettyPrintString());

    EXPECT_EQ(expected->getExecutionSchedule()->prettyPrintString(),
              actual->getExecutionSchedule()->prettyPrintString());

    ASSERT_EQ(expected->getNumReads(), actual->getNumReads());
    for (unsigned int j = 0; j < actual->getNumReads(); ++j) {
      EXPECT_EQ(expected->getReadDataSpace(j),
                actual->getReadDataSpace(j));
      EXPECT_EQ(expected->getReadRelation(j)->prettyPrintString(),
                actual->getReadRelation(j)->prettyPrintString());
    }

    ASSERT_EQ(expected->getNumWrites(), actual->getNumWrites());
    for (unsigned int j = 0; j < actual->getNumWrites(); ++j) {
      EXPECT_EQ(expected->getWriteDataSpace(j),
                actual->getWriteDataSpace(j));
      EXPECT_EQ(expected->getWriteRelation(j)->prettyPrintString(),
                actual->getWriteRelation(j)->prettyPrintString());
    }

    EXPECT_EQ(expected->getAllDebugStr(), actual->getAllDebugStr());
    EXPECT_EQ(expected->isPhiNode(), actual->isPhiNode());
    EXPECT_EQ(expected->isArrayAccess(), actual->isArrayAccess());

    EXPECT_TRUE(*actual == *expected);
  }
};

using SPFComputationDeathTest = SPFComputationTest;

//! Test that the matrix add Computation is built up as expected
TEST_F(SPFComputationTest, matrix_add_correct) {
  std::string code =
      "void matrix_add(int a, int b, int x[a][b], int y[a][b], int sum[a][b]) {\
    int i;\
    int j;\
    for (i = 0; i < a; i++) {\
        for (j = 0; j < b; j++) {\
            sum[i][j] = x[i][j] + y[i][j];\
        }\
    }\
}";

  iegenlib::Computation *computation = buildComputationFromCode(code);

  Computation *expectedComputation = new Computation("matrix_add");
  expectedComputation->addParameter("a", "int");
  expectedComputation->addParameter("b", "int");
  expectedComputation->addParameter("x", "int[][]");
  expectedComputation->addParameter("y", "int[][]");
  expectedComputation->addParameter("sum", "int[][]");

  expectedComputation->addStmt(new iegenlib::Stmt("int i;", "{[0]}", "{[0]->[0,0,0,0,0]}", {}, {}));
  expectedComputation->addStmt(new iegenlib::Stmt("int j;", "{[0]}", "{[0]->[1,0,0,0,0]}", {}, {}));
  expectedComputation
      ->addStmt(new iegenlib::Stmt("sum[i][j] = x[i][j] + y[i][j];",
                                   "{[i,j]: 0 <= i && i < a && 0 <= j && j < b}",
                                   "{[i,j]->[2,i,0,j,0]}",
                                   {{"x", "{[i,j]->[i,j]}"}, {"y", "{[i,j]->[i,j]}"}},
                                   {{"sum", "{[i,j]->[i,j]}"}}));

  expectComputationsEqual(computation, expectedComputation);
}

TEST_F(SPFComputationTest, forward_solve_correct) {
  std::string code =
      "int forward_solve(int n, int l[n][n], double b[n], double x[n]) {\
    int i;\
    for (i = 0; i < n; i++) {\
        x[i] = b[i];\
    }\
\
    int j;\
    for (j = 0; j < n; j++) {\
        x[j] /= l[j][j];\
        for (i = j + 1; i < n; i++) {\
            if (l[i][j] > 0) {\
                x[i] -= l[i][j] * x[j];\
            }\
        }\
    }\
\
    return 0;\
}";

  iegenlib::Computation *computation = buildComputationFromCode(code);

  Computation *expectedComputation = new Computation("forward_solve");
  expectedComputation->addParameter("n", "int");
  expectedComputation->addParameter("l", "int[][]");
  expectedComputation->addParameter("b", "double[]");
  expectedComputation->addParameter("x", "double[]");

  expectedComputation->addStmt(new iegenlib::Stmt("int i;", "{[0]}", "{[0]->[0,0,0,0,0]}", {}, {}));
  expectedComputation
      ->addStmt(new iegenlib::Stmt("x[i] = b[i];",
                                   "{[i]: 0 <= i && i < n}",
                                   "{[i]->[1,i,0,0,0]}",
                                   {{"b", "{[i]->[i]}"}},
                                   {{"x", "{[i]->[i]}"}}));
  expectedComputation->addStmt(new iegenlib::Stmt("int j;", "{[0]}", "{[0]->[2,0,0,0,0]}", {}, {}));
  expectedComputation
      ->addStmt(new iegenlib::Stmt("x[j] /= l[j][j];",
                                   "{[j]: 0 <= j && j < n}",
                                   "{[j]->[3,j,0,0,0]}",
                                   {{"x", "{[j]->[j]}"}, {"l", "{[j]->[j,j]}"}},
                                   {{"x", "{[j]->[j]}"}}));
  expectedComputation->addStmt(new iegenlib::Stmt("x[i] -= l[i][j] * x[j];",
                                                  "{[j,i]: 0 <= j && j < n && j + 1 <= i && i < n && l(i,j) > 0}",
                                                  "{[j,i]->[3,j,1,i,0]}",
                                                  {{"x", "{[j,i]->[i]}"},
                                                   {"l", "{[j,i]->[i,j]}"},
                                                   {"x", "{[j,i]->[j]}"}},
                                                  {{"x", "{[j,i]->[i]}"}}));
  expectedComputation->addStmt(new iegenlib::Stmt("return 0;", "{[0]}", "{[0]->[4,0,0,0,0]}", {}, {}));

  expectComputationsEqual(computation, expectedComputation);
}

TEST_F(SPFComputationTest, csr_spmv_correct) {
  std::string code =
      "\
int CSR_SpMV(int a, int N, int A[a], int index[N + 1], int col[a], int x[N], int product[N]) {\
    int i;\
    int k;\
    for (i = 0; i < N; i++) {\
        for (k = index[i]; k < index[i + 1]; k++) {\
            product[i] += A[k] * x[col[k]];\
        }\
    }\
\
    return 0;\
}\
";

  iegenlib::Computation *computation = buildComputationFromCode(code);

  Computation *expectedComputation = new Computation("CSR_SpMV");
  expectedComputation->addParameter("a", "int");
  expectedComputation->addParameter("N", "int");
  expectedComputation->addParameter("A", "int[]");
  expectedComputation->addParameter("index", "int[]");
  expectedComputation->addParameter("col", "int[]");
  expectedComputation->addParameter("x", "int[]");
  expectedComputation->addParameter("product", "int[]");

  expectedComputation->addStmt(new iegenlib::Stmt("int i;", "{[0]}", "{[0]->[0,0,0,0,0]}", {}, {}));
  expectedComputation->addStmt(new iegenlib::Stmt("int k;", "{[0]}", "{[0]->[1,0,0,0,0]}", {}, {}));
  expectedComputation->addStmt(new iegenlib::Stmt("product[i] += A[k] * x[col[k]];",
                                                  "{[i,k]: 0 <= i && i < N && index(i) <= k && k < index(i + 1)}",
                                                  "{[i,k]->[2,i,0,k,0]}",
                                                  {{"product", "{[i,k]->[i]}"},
                                                   {"A", "{[i,k]->[k]}"},
                                                   {"col", "{[i,k]->[k]}"},
                                                   {"x", "{[i,k]->[" + replacementVarName + "0]: " +
                                                       replacementVarName + "0 = col(k)}"}},
                                                  {{"product", "{[i,k]->[i]}"}}));
  expectedComputation->addStmt(new iegenlib::Stmt("return 0;", "{[0]}", "{[0]->[3,0,0,0,0]}", {}, {}));

  expectComputationsEqual(computation, expectedComputation);
}

/** Death tests, checking failure on invalid input **/

TEST_F(SPFComputationDeathTest, incorrect_increment_fails) {
  std::string code1 =
      "int a() {\
    int x;\
    for (int i = 0; i < 5; i += 2) {\
        x=i;\
    }\
    return x;\
}";
  ASSERT_DEATH(
      buildComputationFromCode(code1),
      "Invalid increment in for loop -- must increase iterator by 1");

  std::string code2 =
      "int a() {\
    int x;\
    for (int i = 0; i < 5; i--) {\
        x=i;\
    }\
    return x;\
}";
  ASSERT_DEATH(
      buildComputationFromCode(code2),
      "Invalid increment in for loop -- must increase iterator by 1");

  std::string code3 =
      "int a() {\
    int x = 0;\
    for (int i = 0; i < 5; i = i - 1) {\
        x=i;\
    }\
    return x;\
}";
  ASSERT_DEATH(
      buildComputationFromCode(code3),
      "Invalid increment in for loop -- must increase iterator by 1");
}

TEST_F(SPFComputationDeathTest, loop_invariant_violation_fails) {
  std::string code =
      "int a() {\
    int x[5] = {1,2,3,4,5};\
    for (int i = 0; x[i] < 5; i += 1) {\
        x[2] = 3;\
    }\
    return x;\
}";
  ASSERT_DEATH(buildComputationFromCode(code),
               "Code may not modify loop-invariant data space 'x'");
}

TEST_F(SPFComputationDeathTest, unsupported_statement_fails) {
  std::string code =
      "int a() {\
    int x;\
    asdf:\
    for (int i = 0; x[i] < 5; i += 1) {\
        x = 3;\
    }\
    goto asdf;\
    return x;\
}";
  ASSERT_DEATH(buildComputationFromCode(code),
               "Unsupported stmt type LabelStmt");
}

TEST_F(SPFComputationDeathTest, invalid_condition_fails) {
  std::string code1 =
      "int a() {\
    int x = 0;\
    if (x)\
        x = 3;\
    }\
    return x;\
}";
  ASSERT_DEATH(buildComputationFromCode(code1),
               "If statement condition must be a binary operation");

  std::string code2 =
      "int a() {\
    int x = 0;\
    if ((x=0))\
        x = 3;\
    }\
    return x;\
}";
  ASSERT_DEATH(buildComputationFromCode(code2),
               "If statement condition must be a binary operation");

  std::string code3 =
      "int a() {\
    int x = 0;\
    if (x !=0)\
        x = 3;\
    }\
    return x;\
}";
  ASSERT_DEATH(
      buildComputationFromCode(code3),
      "Not-equal conditions are unsupported by SPF: in condition x != 0");
}

//! Set up and run tests
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
