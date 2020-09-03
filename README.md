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
with the hopes of using it to generate optimized code.

It supplants (and was initially a rewrite of)
[PDFG-IR_C_frontend](https://github.com/BoiseState-AdaptLab/PDFG-IR_C_frontend).


Requirements
------------
- **CMake:** Download and install CMake from [here](https://cmake.org/download/),
or however else you prefer.
- **LLVM and Clang:** Follow [these instructions](https://github.com/BoiseState-AdaptLab/learningClangLLVM)
explaining how to install and use LLVM and Clang. In addition, LLVM must be
built with the options `LLVM_ENABLE_EH` and `LLVM_ENABLE_RTTI` set to `ON`.


Building
--------
Clone the repository and navigate to the root of this project (the directory
containing this README). Then run the following:
```bash
$ mkdir build
$ cd build
$ cmake -DLLVM_SRC=/path/to/llvm-project/root ..
$ cmake --build .
```


Usage
-----
```bash
$ ./build/new-pdfg-c mysourcefile.cpp
```
where *mysourcefile.cpp* is the input file.
Example files are provided in the test folder.

Documentation
-------------
The CMake target `docs` generates Doxygen documentation in HTML and LaTeX
formats, outputting to the `docs/` directory. From the project root, run:
```bash
$ cmake --build build --target docs
```

