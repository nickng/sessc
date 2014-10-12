sessc: Session C Programming framework
======================================

[Session C](http://www.doc.ic.ac.uk/~cn06/sessionc/) is a research project at [Department of Computing](http://www.doc.ic.ac.uk/), [Imperial College London](http://www.imperial.ac.uk) with the aim to apply the theory of [Multiparty Session Types](http://www.doc.ic.ac.uk/~yoshida/multiparty) in a natively compiled language for communication safe programming in a high performance computing environment.

Components
----------

The framework consists of:
*  Session C runtime library (libsc) based on [ZeroMQ messaging library](http://www.zeromq.org/)
*  Session Type checker, a static analyser based on [LLVM/clang compiler](http://clang.llvm.org/)
*  Support tools for the [Scribble](http://www.scribble.org/) protocol description language
