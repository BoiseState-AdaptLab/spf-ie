cmake_minimum_required(VERSION 3.9)
project (new-pdfg-c C CXX)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -g -O0 -Wall -Wextra -std=c++14 ")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -DPIC -Xpreprocessor -ftree-vectorize -funroll-all-loops ")
# Enable exceptions to compile IEGenLib
add_compile_options( -fexceptions )

# Input path to llvm directory
option (LLVM_SRC "LLVM source directory")
set (CLANG_SRC "${LLVM_SRC}/clang")

if (NOT EXISTS ${LLVM_SRC})
    message (SEND_ERROR "${LLVM_SRC} is not a directory")
    set (HELP true)
endif()

# Inputer path to Polyhedral directory
option (POLY_SRC "PolyhedralDataflowIR source directory")

if (NOT EXISTS ${POLY_SRC})
    message (SEND_ERROR "${POLY_SRC} is not a directory")
    set (HELP true)
endif()

# Add the location of LLVMConfig.cmake so cmake can find it
list (INSERT CMAKE_PREFIX_PATH 0 "${LLVM_SRC}/build/cmake/modules/CMakeFiles")
find_package (LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list (APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list (APPEND CMAKE_MODULE_PATH "${CLANG_SRC}/cmake/modules")

# Add the location of ClangConfig.cmake so cmake can find it
list (INSERT CMAKE_PREFIX_PATH 0 "${LLVM_SRC}/build/lib/cmake/clang")
find_package (Clang REQUIRED CONFIG)

# add_definitions(${LLVM_DEFINITIONS})
include(AddLLVM)
include(AddClang)

set (PROJECT_SOURCES
    PDFGDriver.cpp
    PDFGLFuncBuilder.hpp
    StmtInfoSet.hpp
    Utils.hpp
    IEGenLib.hpp)
list (TRANSFORM PROJECT_SOURCES PREPEND "src/")

# Sources for PolyhedralDataflowIR repository
set (POLY_SOURCES
     lib/iegenlib/src
     lib/isl lib/isl/imath lib/isl/include lib/isl/bld lib/isl/bld/include lib/gmp
     lib/chill lib/chill/omega/basic/include lib/chill/omega/omega_lib/include
     lib/chill/omega/code_gen/include lib/chill/omega/parser
     lib/gtest  lib/gtest/include
     lib/eigen lib/eigen/unsupported
     lib/taco/include)
list (TRANSFORM POLY_SOURCES PREPEND "${POLY_SRC}/")

add_clang_executable(${CMAKE_PROJECT_NAME} ${PROJECT_SOURCES})

# Add directories to include
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${LLVM_INCLUDE_DIRS}")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${LLVM_SRC}/llvm/include")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${CLANG_INCLUDE_DIRS}")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${POLY_SRC}/src")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC ${POLY_SOURCES})

# Find libraries corresponding to LLVM components we need
llvm_map_components_to_libnames(LLVM_LIBS support)

target_link_libraries(${CMAKE_PROJECT_NAME}
            PRIVATE
            ${LLVM_LIBS}
            clangAST
            clangBasic
            clangSerialization
            clangFrontend
            clangTooling
            clangLex
            m polylib
)

