SPF-IE
==================================



Authors
-------

- Anna Rift
- Tobi Popoola
- Aaron Orenstein

Description
-----------
This tool generates the [Sparse Polyhedral Framework](https://doi.org/10.1016/j.parco.2016.02.004)
representation of C source code (gathering information from Clang's AST), using IEGenLib to handle the mathematical
objects involved. The eventual goal is to use this representation for optimizing transformations, the results of which
can be used to generate more efficient inspector/executor code. SPF-IE is pronounced like "spiffy".

It supplants (and was initially a rewrite of)
[PDFG-IR_C_frontend](https://github.com/BoiseState-AdaptLab/PDFG-IR_C_frontend).


Dependencies
------------

- **CMake:** Download and install CMake from [here](https://cmake.org/download/), or however else you prefer.
- **LLVM and Clang:**
  Follow [these instructions](https://github.com/BoiseState-AdaptLab/learningClangLLVM#installation=)
  explaining how to install and use LLVM and Clang, with a few exceptions:
    - Checkout tag llvmorg-11.0.0 before building.
    - Modify the file `llvm/utils/benchmark/src/benchmark_register.h`
      and add the line `#include <limits>` to the top.
    - Use the following CMake build line:
      `cmake -DLLVM_ENABLE_PROJECTS=clang -DBUILD_SHARED_LIBS=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -G "Unix Makefiles" ../llvm`
    - When you run make, pass the `-j` flag followed by the number of concurrent build threads you want (don't put more
      than the number of cores your computer has). This build takes a very very long time without parallel building.
- **IEGenLib:** Automatically cloned and built from
  [here](https://github.com/BoiseState-AdaptLab/IEGenLib) -- you don't have to do anything for this dependency.
- **Google Test:** Automatically built as a dependency of IEGenLib.

Building
--------

### Docker image (recommended)

[riftember/spf-ie](https://hub.docker.com/r/riftember/spf-ie)

This image contains all dependencies (including the LLVM build), so using it
will save a lot of time.

### Manual build

Requires LLVM is already built as specified above.

Clone the repository and navigate to the root of this project (the directory containing this README). Then run the
following:

```bash
$ mkdir build
$ cd build
$ cmake -DLLVM_SRC=/path/to/llvm-project/root ..
$ cmake --build .
```

Usage
-----

```bash
$ ./build/spf-ie mysourcefile.c --entry-point <target function> [--frontend-only]
```

- *mysourcefile.c* is the input file. Example files are provided in the test folder.
- The `--entry-point` flag is required and specifies which function will be compiled.
- The `--frontend-only` flag is optional and causes spf-ie to just run the compiler frontend and print out the resulting
  Computation IR. Default behavior (without this flag) outputs results of codegen on the generated IR.

Testing
-------
From project root, run:

```bash
$ cmake --build build --target test
```

This will build (if necessary) and execute the project's regression tests.


Documentation
-------------
The CMake target `docs` generates Doxygen documentation in HTML and LaTeX formats, outputting to the `docs/` directory.
From the project root, run:

```bash
$ cmake --build build --target docs
```

Developing inside the container
-------------------------------
One can volume mount the local folder in their pc to any path in the container and run the container.

`$ docker run -t -i -v '/path/to/spf-ie:/spf-ie' riftember/spf-ie /bin/bash`
