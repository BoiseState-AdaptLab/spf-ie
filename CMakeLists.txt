cmake_minimum_required(VERSION 3.9)
project (spf-ie C CXX)

include(ExternalProject)

set (CMAKE_CXX_FLAGS "-g -O0 -Wall -Wextra -std=c++14")

option (LLVM_SRC "LLVM source directory")
set (CLANG_SRC "${LLVM_SRC}/clang")


add_custom_target(docs COMMAND doxygen docs/Doxyfile WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

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

set(LLVM_ENABLE_EH ON)
set(LLVM_ENABLE_RTTI ON)

# get IEGenLib
ExternalProject_Add(iegenlib_in
    GIT_REPOSITORY "https://github.com/CompOpt4Apps/IEGenLib.git"
    GIT_TAG "0cfa0a82fc83cb9fa61376ed98fc2786f6e18cb6"
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/IEGenLib
    BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND "./configure"
)
# add link dirs for IEGenLib and its dependencies
link_directories("lib/IEGenLib/lib/installed/lib")
link_directories("lib/IEGenLib/iegen/lib")


set (PROJECT_SOURCES
    Driver.cpp
    SPFComputationBuilder.cpp
    StmtContext.cpp
    Utils.cpp
    ArrayAccess.cpp
)
list (TRANSFORM PROJECT_SOURCES PREPEND "src/")

add_clang_executable(${CMAKE_PROJECT_NAME} ${PROJECT_SOURCES})
add_dependencies(${CMAKE_PROJECT_NAME} iegenlib_in)

# Add directories to include
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "include")
# LLVM/Clang
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${LLVM_INCLUDE_DIRS}")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${LLVM_SRC}/llvm/include")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "${CLANG_INCLUDE_DIRS}")
# IEGenLib and deps
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "lib/IEGenLib/iegen/include/iegenlib")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "lib/IEGenLib/lib/installed/include")
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PUBLIC "lib/IEGenLib/lib/gtest/include")

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
            iegenlib
            gmp
            isl
)

