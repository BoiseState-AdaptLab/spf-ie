SPF-IE
==================================


Authors
-------
- Anna Rift
- Aaron Orenstein


Description
-----------
This tool generates the [Sparse Polyhedral Framework](https://doi.org/10.1016/j.parco.2016.02.004)
representation of C/C++ source code (gathering information from Clang's AST),
using IEGenLib to handle the mathematical objects involved. The eventual goal
is to use this representation for optimizing transformations, the results of
which can be used to generate more efficient inspector/executor code. SPF-IE
is pronounced like "spiffy".

It supplants (and was initially a rewrite of)
[PDFG-IR_C_frontend](https://github.com/BoiseState-AdaptLab/PDFG-IR_C_frontend).


Dependencies
------------
- **CMake:** Download and install CMake from [here](https://cmake.org/download/),
or however else you prefer.
- **LLVM and Clang:** Follow [these instructions](https://github.com/BoiseState-AdaptLab/learningClangLLVM)
explaining how to install and use LLVM and Clang. In addition, LLVM must be
built with the options `LLVM_ENABLE_EH` and `LLVM_ENABLE_RTTI` set to `ON`.
Developed with LLVM 11 -- using other versions may not work properly.
- **IEGenLib:** Automatically cloned and built from
[here](https://github.com/CompOpt4Apps/IEGenLib) -- you don't have to do
anything for this dependency.


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
$ ./build/spf-ie mysourcefile.cpp
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
