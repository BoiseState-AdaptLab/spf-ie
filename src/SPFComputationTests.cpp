/*!
 * \file SPFComputationTests.cpp
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

const ASTContext* spf_ie::Context;

/*!
 * \class SPFComputationTests
 *
 * \brief Google Test fixture for SPFComputationBuilder.
 *
 * Contains tools for efficiently regression testing the output of the
 * SPFComputationBuilder against expected values for a given piece of code.
 */
class SPFComputationTests : public ::testing::Test {
   protected:
    virtual void SetUp() override {}
    virtual void TearDown() override {}

    //! Build SPFComputations from every function in the provided code.
    std::vector<std::unique_ptr<iegenlib::Computation>>
    buildSPFComputationsFromCode(std::string code) {
        std::unique_ptr<ASTUnit> AST = tooling::buildASTFromCode(
            code, "input.cpp", std::make_shared<PCHContainerOperations>());
        Context = &AST->getASTContext();

        std::vector<std::unique_ptr<iegenlib::Computation>> computations;
        SPFComputationBuilder builder;
        for (auto it : Context->getTranslationUnitDecl()->decls()) {
            FunctionDecl* func = dyn_cast<FunctionDecl>(it);
            if (func && func->doesThisDeclarationHaveABody()) {
                computations.push_back(
                    builder.buildComputationFromFunction(func));
            }
        }
        return computations;
    }

    //! Use assertions/expectations to compare an SPFComputation to
    //! expected values.
    void compareComputationToExpectations(
        const iegenlib::Computation* computation, int expectedNumStmts,
        const std::unordered_set<std::string>& expectedDataSpaces,
        const std::vector<std::string>& expectedIterSpaces,
        const std::vector<std::string>& expectedExecSchedules,
        const std::vector<std::vector<std::pair<std::string, std::string>>>&
            expectedReads,
        const std::vector<std::vector<std::pair<std::string, std::string>>>&
            expectedWrites) {
        // sanity check that we have the correct number of expected values
        ASSERT_EQ(expectedIterSpaces.size(), expectedNumStmts);
        ASSERT_EQ(expectedExecSchedules.size(), expectedNumStmts);
        ASSERT_EQ(expectedReads.size(), expectedNumStmts);
        ASSERT_EQ(expectedWrites.size(), expectedNumStmts);

        EXPECT_EQ(computation->getDataSpaces(), expectedDataSpaces);
        ASSERT_EQ(computation->getNumStmts(), expectedNumStmts);
        for (int i = 0; i < expectedNumStmts; ++i) {
            const iegenlib::Stmt* current = computation->getStmt(i);
            // include statement number/source in trace
            SCOPED_TRACE("S" + std::to_string(i) + ": " +
                         current->getStmtSourceCode());
            // iteration space
            auto* expectedIterSpace = new iegenlib::Set(expectedIterSpaces[i]);
            EXPECT_EQ(current->getIterationSpace()->prettyPrintString(),
                      expectedIterSpace->prettyPrintString());
            delete expectedIterSpace;
            // execution schedule
            auto* expectedExecSchedule =
                new iegenlib::Relation(expectedExecSchedules[i]);
            EXPECT_EQ(current->getExecutionSchedule()->prettyPrintString(),
                      expectedExecSchedule->prettyPrintString());
            delete expectedExecSchedule;
            // reads
            auto dataReads = current->getDataReads();
            ASSERT_EQ(dataReads.size(), expectedReads[i].size());
            unsigned int j = 0;
            for (const auto& it_read : dataReads) {
                SCOPED_TRACE("read " + std::to_string(j));
                EXPECT_EQ(it_read.first, expectedReads[i][j].first);
                auto* expectedReadRel =
                    new iegenlib::Relation(expectedReads[i][j].second);
                EXPECT_EQ(it_read.second->prettyPrintString(),
                          expectedReadRel->prettyPrintString());
                delete expectedReadRel;
                j++;
            }
            // writes
            auto dataWrites = current->getDataWrites();
            ASSERT_EQ(dataWrites.size(), expectedWrites[i].size());
            j = 0;
            for (const auto& it_write : dataWrites) {
                SCOPED_TRACE("write " + std::to_string(j));
                EXPECT_EQ(it_write.first, expectedWrites[i][j].first);
                auto* expectedWriteRel =
                    new iegenlib::Relation(expectedWrites[i][j].second);
                EXPECT_EQ(it_write.second->prettyPrintString(),
                          expectedWriteRel->prettyPrintString());
                delete expectedWriteRel;
                j++;
            }
        }
    }
};

//! Test that the matrix add Computation is built up as expected
TEST_F(SPFComputationTests, matrix_add) {
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
    ASSERT_EQ(computations.size(), 1);
    iegenlib::Computation* computation = computations.back().get();

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

TEST_F(SPFComputationTests, forward_solve) {
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
    ASSERT_EQ(computations.size(), 1);
    iegenlib::Computation* computation = computations.back().get();

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
        "{[]->[0,0,0,0,0]}",  "{[i]->[1,i,0,0,0]}",   "{[]->[2,0,0,0,0]}",
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

//! Set up and run tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
