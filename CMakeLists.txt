cmake_minimum_required(VERSION 3.9)
project (new-pdfg-c C CXX)

set (CMAKE_CXX_FLAGS "-g -O0 -Wall -Wextra -std=c++14")


option (LLVM_SRC "LLVM source directory")
set (CLANG_SRC "${LLVM_SRC}/clang")

# Add the location of LLVMConfig.cmake so cmake can find it
list (INSERT CMAKE_PREFIX_PATH 0 "${LLVM_SRC}/build/cmake/modules/CMakeFiles")
# list (APPEND CMAKE_PREFIX_PATH "${LLVM_SRC}/llvm/cmake")
# list (APPEND CMAKE_PREFIX_PATH "${CLANG_SRC}/cmake")
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
    PDFGLBuilder.hpp
    IterSpace.hpp
    Utils.hpp)
list (TRANSFORM PROJECT_SOURCES PREPEND "src/")
add_clang_executable(${CMAKE_PROJECT_NAME} ${PROJECT_SOURCES})

# Add directories to include
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${LLVM_INCLUDE_DIRS}")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${LLVM_SRC}/llvm/include")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${CLANG_INCLUDE_DIRS}")

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
)

