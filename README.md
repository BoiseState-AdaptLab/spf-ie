Clang AST Visitor
===========

Anna Rift  
Aaron Orenstein

Installation
------------
#### LLVM/Clang:

Follow the instructions at https://clang.llvm.org/get_started.html with one
modification: add the flag `-DBUILD_SHARED_LIBS=ON` to the CMake command.

The installation takes around 15GiB of disk space and a similar amount of
memory. If `ld` is killed unexpectedly during build, memory is likely the
issue. This can be resolved by adding swap space or using a computer with more
memory. On Unix-like systems, it may be possible to consume less memory by
using the `gold` linker instead of `ld`; to do this, add the
`-DLLVM_USE_LINKER=gold` flag to the CMake command as per the instructions
[here](http://llvm.org/docs/CMake.html).

You may wish to add */path/to/llvm-project/build/bin* to your `PATH`, so that
the executables produced by build are easily accessible from the command line.
This is more important if you have multiple LLVM installations, and want to be
sure that you use this one when invoking tools like `llvm-config`.

#### PolyhedralDataflowIR:

Get the PolyhedralDataflowIR repository
[here](https://github.com/BoiseState-AdaptLab/PolyhedralDataflowIR)
and follow installation instructions on the README.


Setup
-----
Requires LLVM and Clang and PolyhedralDataflowIR installation, as well as CMake.

Clone the repository and navigate to the root of this project (the directory
containing this README). Then run the following:
```bash
$ cmake -DLLVM_SRC=/path/to/llvm-project/root -DPOLY_SRC=/path/to/polyhedral/root .
$ make
```

Usage
-----
```bash
$ ./new-pdfg-c mysourcefile.cpp 
```
where *mysourcefile.cpp* is the input file.
Example files are provided in the test folder
