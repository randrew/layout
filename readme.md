Layout
======

A simple/fast stacking box layout library. It's useful for calculating layouts
for things like 2D user interfaces. It compiles as C99 or C++. It's tested with
gcc (mingw64) and VS2015, but should probably build with Clang/LLVM with a few
changes.

It comes with a small set of tests as a build target, along with a primitive
benchmark and example usage of the library as a Lua .dll module.

Building is handled by [GENie](https://github.com/bkaradzic/GENie), but no
executable binaries are included in this source repository. You will need to
download (or build yourself) a GENie executable and place it in your path or at
the root of this repository tree. If you want to build on a platform other than
Windows, platform, you'll likely need to modify genie.lua to add compatibility
for that platform.

However, if you want to use Layout in your own project, you can probably just
copy layout.h and layout.c into your project's source tree. You don't
necessarily have to build it as a shared library and link against it. There are
no external dependencies.

*Layout* is based on the nice library
[oui](https://bitbucket.org/duangle/oui-blendish) by duangle. Unlike *oui*,
*Layout* does not handle anything related to user input, focus, or UI state.

Building
========

You will first need to get (or make) a GENie binary and place it in your path
or at the root of this repository.

Linux:  
https://github.com/bkaradzic/bx/raw/master/tools/bin/linux/genie

OSX:  
https://github.com/bkaradzic/bx/raw/master/tools/bin/darwin/genie

Windows:  
https://github.com/bkaradzic/bx/raw/master/tools/bin/windows/genie.exe

Visual Studio 2015
------------------

> genie.exe vs2015

MinGW
-----

> ./genie.exe gmake && mingw32-make.exe -C build/gmake tests config=release64
