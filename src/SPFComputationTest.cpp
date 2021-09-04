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
  void SetUp() override {}
  void TearDown() override {}

  std::string replacementVarName = REPLACEMENT_VAR_BASE_NAME;

  //! Build SPFComputations from every function in the provided code.
  static std::vector<std::unique_ptr<iegenlib::Computation>>
  buildSPFComputationsFromCode(std::string code) {
	std::unique_ptr<ASTUnit> AST = tooling::buildASTFromCode(
		code, "test_input.cpp", std::make_shared<PCHContainerOperations>());
	Context = &AST->getASTContext();

	std::vector<std::unique_ptr<iegenlib::Computation>> computations;
	SPFComputationBuilder builder;
	for (auto it: Context->getTranslationUnitDecl()->decls()) {
	  auto *func = dyn_cast<FunctionDecl>(it);
	  if (func && func->doesThisDeclarationHaveABody()) {
		computations.push_back(
			builder.buildComputationFromFunction(func));
	  }
	}
	return computations;
  }

  //! Use assertions/expectations to compare an SPFComputation to
  //! expected values.
  static void compareComputationToExpectations(
	  iegenlib::Computation *computation, int expectedNumStmts,
	  const std::unordered_set<std::string> &expectedDataSpaces,
	  const std::vector<std::string> &expectedIterSpaces,
	  const std::vector<std::string> &expectedExecSchedules,
	  const std::vector<std::vector<std::pair<std::string, std::string>>> &
	  expectedReads,
	  const std::vector<std::vector<std::pair<std::string, std::string>>> &
	  expectedWrites) {
	// sanity check that we have the correct number of expected values
	ASSERT_EQ(expectedNumStmts, expectedIterSpaces.size());
	ASSERT_EQ(expectedNumStmts, expectedExecSchedules.size());
	ASSERT_EQ(expectedNumStmts, expectedReads.size());
	ASSERT_EQ(expectedNumStmts, expectedWrites.size());

	EXPECT_EQ(expectedDataSpaces, computation->getDataSpaces());
	ASSERT_EQ(expectedNumStmts, computation->getNumStmts());
	for (int i = 0; i < expectedNumStmts; ++i) {
	  iegenlib::Stmt *const current = computation->getStmt(i);
	  // include statement number/source in trace
	  SCOPED_TRACE("S" + std::to_string(i) + ": " +
		  current->getStmtSourceCode());
	  // iteration space
	  auto *expectedIterSpace = new iegenlib::Set(expectedIterSpaces[i]);
	  EXPECT_EQ(expectedIterSpace->prettyPrintString(),
				current->getIterationSpace()->prettyPrintString());
	  delete expectedIterSpace;
	  // execution schedule
	  auto *expectedExecSchedule =
		  new iegenlib::Relation(expectedExecSchedules[i]);
	  EXPECT_EQ(expectedExecSchedule->prettyPrintString(),
				current->getExecutionSchedule()->prettyPrintString());
	  delete expectedExecSchedule;
	  // reads
	  auto dataReads = current->getDataReads();
	  ASSERT_EQ(expectedReads[i].size(), dataReads.size());
	  unsigned int j = 0;
	  for (const auto &it_read: dataReads) {
		SCOPED_TRACE("read " + std::to_string(j));
		EXPECT_EQ(expectedReads[i][j].first, it_read.first);
		auto *expectedReadRel =
			new iegenlib::Relation(expectedReads[i][j].second);
		EXPECT_EQ(expectedReadRel->prettyPrintString(),
				  it_read.second->prettyPrintString());
		delete expectedReadRel;
		j++;
	  }
	  // writes
	  auto dataWrites = current->getDataWrites();
	  ASSERT_EQ(expectedWrites[i].size(), dataWrites.size());
	  j = 0;
	  for (const auto &it_write: dataWrites) {
		SCOPED_TRACE("write " + std::to_string(j));
		EXPECT_EQ(expectedWrites[i][j].first, it_write.first);
		auto *expectedWriteRel =
			new iegenlib::Relation(expectedWrites[i][j].second);
		EXPECT_EQ(expectedWriteRel->prettyPrintString(),
				  it_write.second->prettyPrintString());
		delete expectedWriteRel;
		j++;
	  }
	}
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

  std::vector<std::unique_ptr<iegenlib::Computation>> computations =
	  buildSPFComputationsFromCode(code);
  ASSERT_EQ(1, computations.size());
  iegenlib::Computation *computation = computations.back().get();

  // expected values for the computation
  unsigned int expectedNumStmts = 3;
  std::unordered_set<std::string> expectedDataSpaces = {"sum", "x", "y"};
  std::vector<std::string> expectedIterSpaces = {
	  "{[]}", "{[]}", "{[i,j]: 0 <= i && i < a && 0 <= j && j < b}"};
  std::vector<std::string> expectedExecSchedules = {
	  "{[]->[0,0,0,0,0]}", "{[]->[1,0,0,0,0]}", "{[i,j]->[2,i,0,j,0]}"};
  std::vector<std::vector<std::pair<std::string, std::string>>>
	  expectedReads = {
	  {}, {}, {{"x", "{[i,j]->[i,j]}"}, {"y", "{[i,j]->[i,j]}"}}};
  std::vector<std::vector<std::pair<std::string, std::string>>>
	  expectedWrites = {{}, {}, {{"sum", "{[i,j]->[i,j]}"}}};

  compareComputationToExpectations(
	  computation, expectedNumStmts, expectedDataSpaces, expectedIterSpaces,
	  expectedExecSchedules, expectedReads, expectedWrites);
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

  std::vector<std::unique_ptr<iegenlib::Computation>> computations =
	  buildSPFComputationsFromCode(code);
  ASSERT_EQ(1, computations.size());
  iegenlib::Computation *computation = computations.back().get();

  unsigned int expectedNumStmts = 6;
  std::unordered_set<std::string> expectedDataSpaces = {"x", "b", "l"};
  std::vector<std::string> expectedIterSpaces = {
	  "{[]}",
	  "{[i]: 0 <= i && i < n}",
	  "{[]}",
	  "{[j]: 0 <= j && j < n}",
	  "{[j,i]: 0 <= j && j < n && j + 1 <= i && i < n && l(i,j) > 0}",
	  "{[]}"};
  std::vector<std::string> expectedExecSchedules = {
	  "{[]->[0,0,0,0,0]}", "{[i]->[1,i,0,0,0]}", "{[]->[2,0,0,0,0]}",
	  "{[j]->[3,j,0,0,0]}", "{[j,i]->[3,j,1,i,0]}", "{[]->[4,0,0,0,0]}"};
  std::vector<std::vector<std::pair<std::string, std::string>>>
	  expectedReads = {{},
					   {{"b", "{[i]->[i]}"}},
					   {},
					   {{"x", "{[j]->[j]}"}, {"l", "{[j]->[j,j]}"}},
					   {{"x", "{[j,i]->[i]}"},
						{"l", "{[j,i]->[i,j]}"},
						{"x", "{[j,i]->[j]}"}},
					   {}};
  std::vector<std::vector<std::pair<std::string, std::string>>>
	  expectedWrites = {{},
						{{"x", "{[i]->[i]}"}},
						{},
						{{"x", "{[j]->[j]}"}},
						{{"x", "{[j,i]->[i]}"}},
						{}};

  compareComputationToExpectations(
	  computation, expectedNumStmts, expectedDataSpaces, expectedIterSpaces,
	  expectedExecSchedules, expectedReads, expectedWrites);
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

  std::vector<std::unique_ptr<iegenlib::Computation>> computations =
	  buildSPFComputationsFromCode(code);
  ASSERT_EQ(1, computations.size());
  iegenlib::Computation *computation = computations.back().get();

  unsigned int expectedNumStmts = 4;
  std::unordered_set<std::string> expectedDataSpaces = {"product", "A", "col",
														"x"};
  std::vector<std::string> expectedIterSpaces = {
	  "{[]}", "{[]}",
	  "{[i,k]: 0 <= i && i < N && index(i) <= k && k < index(i + 1)}",
	  "{[]}"};
  std::vector<std::string> expectedExecSchedules = {
	  "{[]->[0,0,0,0,0]}", "{[]->[1,0,0,0,0]}", "{[i,k]->[2,i,0,k,0]}",
	  "{[]->[3,0,0,0,0]}"};
  std::vector<std::vector<std::pair<std::string, std::string>>>
	  expectedReads = {{},
					   {},
					   {{"product", "{[i,k]->[i]}"},
						{"A", "{[i,k]->[k]}"},
						{"col", "{[i,k]->[k]}"},
						{"x", "{[i,k]->[" + replacementVarName + "0]: " +
							replacementVarName + "0 = col(k)}"}},
					   {}};
  std::vector<std::vector<std::pair<std::string, std::string>>>
	  expectedWrites = {{}, {}, {{"product", "{[i,k]->[i]}"}}, {}};

  compareComputationToExpectations(
	  computation, expectedNumStmts, expectedDataSpaces, expectedIterSpaces,
	  expectedExecSchedules, expectedReads, expectedWrites);
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
	  buildSPFComputationsFromCode(code1),
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
	  buildSPFComputationsFromCode(code2),
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
	  buildSPFComputationsFromCode(code3),
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
  ASSERT_DEATH(buildSPFComputationsFromCode(code),
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
  ASSERT_DEATH(buildSPFComputationsFromCode(code),
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
  ASSERT_DEATH(buildSPFComputationsFromCode(code1),
			   "If statement condition must be a binary operation");

  std::string code2 =
	  "int a() {\
    int x = 0;\
    if ((x=0))\
        x = 3;\
    }\
    return x;\
}";
  ASSERT_DEATH(buildSPFComputationsFromCode(code2),
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
	  buildSPFComputationsFromCode(code3),
	  "Not-equal conditions are unsupported by SPF: in condition x != 0");
}

//! Set up and run tests
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
