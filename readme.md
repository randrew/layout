Layout
======

A simple/fast stacking box layout library. It's useful for calculating layouts
for things like 2D user interfaces. It compiles as C99 or C++. It's tested with
gcc (mingw64), VS2015, and clang/LLVM. *Layout* has no external dependencies.

*Layout* comes with a small set of tests as a build target, along with a
primitive benchmark and example usage of the library as a Lua .dll module.

Building the tests, benchmarks and Lua module are handled by
[GENie](https://github.com/bkaradzic/GENie), but no executable binaries for
GENie are included in this source repository. You will need to download (or
build yourself) a GENie executable and place it in your path or at the root of
this repository tree. If you want to build on a platform other than Windows,
you'll likely need to modify genie.lua for compatibility. Pull requests are
welcome.

However, if you want to use *Layout* in your own project, you can probably copy
layout.h and layout.c into your project's source tree and use your own build
system. You don't have to separately build *Layout* as a shared library and
link against it.

If you want to enforce SSE alignment for the result values, you'll probably
want to add or tweak alignment specifiers for some types (or union them with
m128) in layout.h. The code is simple and should be easy to modify for your
purposes.

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

```
genie.exe vs2015
```

MinGW
-----

```
./genie.exe gmake && mingw32-make.exe -C build/gmake tests config=release64
```
