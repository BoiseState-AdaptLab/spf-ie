Polyhedral Dataflow Graph Compiler
==================================

Authors
-------
- Anna Rift
- Aaron Orenstein


Description
-----------
This compiler collects information from C/C++ source code for
[the Polyhedral Dataflow Graph Representation](https://github.com/BoiseState-AdaptLab/PolyhedralDataflowIR),
with the hopes of using it to generate optimizationed code.

It supplants (and was initially a rewrite of)
[PDFG-IR_C_frontend](https://github.com/BoiseState-AdaptLab/PDFG-IR_C_frontend).


Requirements
------------
- **LLVM and Clang:** Follow [these instructions](https://github.com/BoiseState-AdaptLab/learningClangLLVM)
explaining how to install and use LLVM and Clang.
- **PolyhedralDataflowIR:** Get the PolyhedralDataflowIR repository
[here](https://github.com/BoiseState-AdaptLab/PolyhedralDataflowIR)
and follow the installation instructions on the README.
- **CMake:** Download and install CMake from [here](https://cmake.org/download/),
or however else you prefer.


Building
--------
Clone the repository and navigate to the root of this project (the directory
containing this README). Then run the following:
```bash
$ mkdir build
$ cd build
$ cmake -DLLVM_SRC=/path/to/llvm-project/root -DPOLY_SRC=/path/to/polyhedral/root ..
$ cmake --build .
```


Usage
-----
```bash
$ ./build/new-pdfg-c mysourcefile.cpp
```
where *mysourcefile.cpp* is the input file.
Example files are provided in the test folder.

