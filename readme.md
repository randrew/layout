Layout
======

A simple/fast stacking box layout library. It's useful for calculating layouts
for things like 2D user interfaces. It compiles as C99 or C++. It's tested with
gcc (mingw64) and VS2015, but should probably build with Clang/LLVM with a few
changes.

It comes with a small set of tests as a build target, along with a primitive
benchmark and example usage of the library as a Lua .dll module.

Building is handled by [GENie](https://github.com/bkaradzic/GENie), but only a
Windows binary is included in this repository. If you want to build on another
platform, you'll need to get GENie for that platform, and you'll also likely
need to modify genie.lua.

If you're using this code in your own project, you can probably just copy
layout.h and layout.c into your project tree. You don't necessarily have to
build it as a shared library and include it that way. There are no external
dependencies.

*Layout* is based on the nice library
[oui](https://bitbucket.org/duangle/oui-blendish) by duangle. Unlike *oui*,
*Layout* does not handle anything related to user input, focus, or UI state.

Building
========

Visual Studio 2015
------------------

> genie.exe vs2015

MinGW
-----

> ./genie.exe gmake && mingw32-make.exe -C build/gmake tests config=release64
