cmake_minimum_required(VERSION 3.9)
project(spf-ie C CXX)


include(ExternalProject)

set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Wextra -std=c++14")

option(LLVM_SRC "LLVM source directory")
set(CLANG_SRC "${LLVM_SRC}/clang")


add_custom_target(docs COMMAND doxygen docs/Doxyfile WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


# set up Clang/LLVM
# Add the location of LLVMConfig.cmake so cmake can find it
list(INSERT CMAKE_PREFIX_PATH 0 "${LLVM_SRC}/build/cmake/modules/CMakeFiles")
# list (APPEND CMAKE_PREFIX_PATH "${LLVM_SRC}/llvm/cmake")
# list (APPEND CMAKE_PREFIX_PATH "${CLANG_SRC}/cmake")
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CLANG_SRC}/cmake/modules")

# Add the location of ClangConfig.cmake so cmake can find it
list(INSERT CMAKE_PREFIX_PATH 0 "${LLVM_SRC}/build/lib/cmake/clang")
find_package(Clang REQUIRED CONFIG)

# add_definitions(${LLVM_DEFINITIONS})
include(AddLLVM)
include(AddClang)

set(LLVM_ENABLE_EH ON)
set(LLVM_ENABLE_RTTI ON)


# get IEGenLib
ExternalProject_Add(iegenlib_in
        GIT_REPOSITORY "https://github.com/BoiseState-AdaptLab/IEGenLib.git"
        GIT_TAG "fe4e47da"
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/IEGenLib
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/lib/installed
        BUILD_COMMAND ${MAKE})
# add link dirs for IEGenLib and its dependencies
link_directories("lib/installed/lib")
link_directories("lib/IEGenLib/lib/installed/lib")
link_directories("lib/IEGenLib/iegen/lib")
# gtest
ExternalProject_Get_property(iegenlib_in BINARY_DIR)
link_directories("${BINARY_DIR}/lib/gtest")


# gather up project sources
set(PROJECT_SOURCES
        ComputationBuilder.cpp
        PositionContext.cpp
        ExecSchedule.cpp
        DataAccessHandler.cpp
        Utils.cpp
        )
list(TRANSFORM PROJECT_SOURCES PREPEND "src/")

# make executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_clang_library(${CMAKE_PROJECT_NAME}_lib ${PROJECT_SOURCES})

add_clang_executable(${CMAKE_PROJECT_NAME} src/Driver.cpp)
add_dependencies(${CMAKE_PROJECT_NAME} iegenlib_in ${CMAKE_PROJECT_NAME}_lib)

add_clang_executable("${CMAKE_PROJECT_NAME}_t" EXCLUDE_FROM_ALL src/ComputationBuilderTest.cpp)
add_dependencies("${CMAKE_PROJECT_NAME}_t" iegenlib_in ${CMAKE_PROJECT_NAME}_lib)
add_test("${CMAKE_PROJECT_NAME}_tests" "${CMAKE_PROJECT_NAME}_t")

add_custom_command(OUTPUT spfie_test
        COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_PROJECT_NAME}_t"
        DEPENDS "${CMAKE_PROJECT_NAME}_t"
        COMMENT "Run spf-ie tests"
        )
add_custom_target(test DEPENDS spfie_test)

# Add directories to include
include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "include")
# LLVM/Clang
include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${LLVM_INCLUDE_DIRS}")
include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${LLVM_SRC}/llvm/include")
include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${CLANG_INCLUDE_DIRS}")
# IEGenLib and deps
include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "lib/installed/include/iegenlib")
include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "lib/IEGenLib/lib/installed/include")
include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "lib/IEGenLib/lib/gtest/include")


# setup libraries
# Find libraries corresponding to LLVM components we need
llvm_map_components_to_libnames(LLVM_LIBS support)

set(${CMAKE_PROJECT_NAME}_LIBS
        ${CMAKE_PROJECT_NAME}_lib
        ${LLVM_LIBS}
        clangAST
        clangBasic
        clangSerialization
        clangFrontend
        clangTooling
        clangLex
        iegenlib
        gtest
        gmp
        isl
        codegen
        omega
        )

target_link_libraries(${CMAKE_PROJECT_NAME}
        PRIVATE
        ${${CMAKE_PROJECT_NAME}_LIBS}
        )

target_link_libraries("${CMAKE_PROJECT_NAME}_t"
        PRIVATE
        ${${CMAKE_PROJECT_NAME}_LIBS}
        )

