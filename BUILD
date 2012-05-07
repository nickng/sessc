sessc: Session C Programming framework
======================================

Dependencies
------------
   -   POSIX-compatible lex/yacc
   -   ZeroMQ 2.1 development headers and shared libraries

Building
--------

To build only the scribble-tool utility, run:

$ make scribble-tool


To build the scribble-tool utility and the runtime library, run:

$ make runtime


To build the type checker, you will need to first get and build LLVM/clang (http://clang.llvm.org/) from source,
then copy the source of the type checker under the clang source tree:

$ mkdir $(LLVM_ROOT)/tools/clang/examples/SessionTypeChecker
$ cp src/typechecker/SessionTypeChecker.cpp $(LLVM_ROOT)/tools/clang/examples/SessionTypeChecker/
$ cp src/typechecker/Makefile $(LLVM_ROOT)/tools/clang/examples/SessionTypeChecker/
$ cd $(LLVM_ROOT)/tools/clang/examples/SessionTypeChecker; make

Then run the type checker as a clang plugin:

$(LLVM_ROOT)/bin/clang -Xclang -load -Xclang $(LLVM_ROOT)/lib/libSessionTypeChecker.so -Xclang -add-plugin -Xclang sess-type-check $(CFLAGS) source.c $(LDFLAGS)
