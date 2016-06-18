Layout
======

A simple/fast stacking box layout library. It's useful for calculating layouts
for things like 2D user interfaces. It compiles as C99 or C++. It's tested with
gcc (mingw64), VS2015, and clang/LLVM. There are only two important files,
[layout.h](layout.h) and [layout.c](layout.c).

![](https://raw.githubusercontent.com/wiki/randrew/layoutexample/ui_anim_small.gif)

*Layout* has no external dependencies, but does use `stdlib.h` and `string.h`
for `realloc` and `memset`. If your own project does not or cannot use these,
you can easily exchange them for something else. Only a few lines will need to
be edited.

*Layout* comes with a small set of tests as a build target, along with a
primitive benchmark and example usage of the library as a Lua .dll module.
However, if you want to use *Layout* in your own project, you can probably copy
[layout.h](layout.h) and [layout.c](layout.c) into your project's source tree
and use your own build system. You don't have to separately build *Layout* as a
shared library and link against it.

Building the tests, benchmarks and Lua module are handled by
[GENie](https://github.com/bkaradzic/GENie), but no executable binaries for
GENie are included in this source repository. [Download links for binary builds
of GENie are listed below](#download-genie). You will need to download (or
build yourself) a GENie executable and place it in your path or at the root of
this repository tree. If you want to build on a platform other than Windows,
you'll likely need to modify [genie.lua](genie.lua) for compatibility. Feel
free to open issues or pull requests.

*Layout* is based on the nice library
[oui](https://bitbucket.org/duangle/oui-blendish) by
[duangle](https://twitter.com/duangle). Unlike *oui*, *Layout* does not handle
anything related to user input, focus, or UI state.

Building
========

If you just want to use *Layout* in your own project, you can simply copy
[layout.h](layout.h) and [layout.c](layout.c) directly into your project. Take
a look at [genie.lua](genie.lua) for some recommended compiler and linker
options.

If you want to build *Layout*'s tests, benchmarks, or example Lua module, you
will first need to get (or make) a GENie binary and place it in your path or at
the root of this repository.

Download GENie
--------------

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
start build/vs2015/layout.sln
```

GCC/MinGW/Clang
---------------

```
./genie gmake
```

and then run your `make` in the directory `build/gmake`. You will need to
specify a target and config. Here is an example for building the `tests` target
in Windows with the 64-bit release configuration using mingw64 in a bash-like
shell (for example, git bash):


```
./genie.exe gmake && mingw32-make.exe -C build/gmake tests config=release64
```

Options
-------

You can choose to build *Layout* to use either integer (int16) or floating
point (float) coordinates. Integer is the default, because UI and other 2D
layouts do not often use units smaller than a single point when aligning and
positioning elements. You can choose to use floating point instead of integer
by defining `LAY_FLOAT`. If you are building the tests, benchmarks, or Lua
module for *Layout*, you can configure this when you invoke GENie:

```
./genie gmake --coords=float
```

or if you want to specify integer (the default):

```
./genie gmake --coords=integer
```

If you are building *Layout* to use floating point coordinates, and if you want
to enforce SSE alignment for the vector coordinate types, you'll probably want
to add or tweak alignment specifiers for some types (or typedef them to m128)
in [layout.h](layout.h). The code is simple and should be easy to modify for
your purposes. If you do this, and if you also change the code to use your own
custom allocator, you might also need to guarantee that the starting addresses
of the buffers given to *Layout* by your allocator are 16-byte aligned. If you
do all of that, and if you build *Layout* as a shared library or a static
library without linker optimizations enabled in MSVC, you might want to also
consider using `__vectorcall`, which may help reduce overhead when calling
functions which receive or return SSE values.
