local action = _ACTION or ""

newaction {
    trigger = "clean",
    description = "Clean everything",
    execute = function ()
        os.rmdir("./build")
    end
}

newaction {
    trigger = "asm",
    description = "Emit GCC assembly output for a file",
    -- Hard-coded for windows+gmake, probably has shell quote bugs
    execute = function ()
        os.execute("gcc -ggdb1 -Ofast -S -fno-stack-check -fno-dwarf2-cfi-asm -fno-asynchronous-unwind-tables -masm=intel -march=nehalem -fverbose-asm -DNDEBUG " .. _ARGS[1])
    end
}

newoption {
    trigger = "coords",
    value = "coordtype",
    description = "Use either integer or floating point for coordinates and sizes",
    allowed = {
        { "integer", "16-bit signed integer" },
        { "float", "32-bit floating point" },
    }
}

local targetDir = path.join("./build", _ACTION, "bin")

function incl_luajit()
        configuration {}
            includedirs {
                "./thirdparty/LuaJIT-2.0.0/src",
            }
            -- Note: you will need to manually rebuild LuaJIT with the correct
            -- compiler (mingw, msvc) first
            libdirs {
                "./thirdparty/LuaJIT-2.0.0/src",
            }
            -- lua51.dll
            links { "lua51" }
end

function as_console_app()
            kind "ConsoleApp"
            -- Needed in mingw when building console app, genie/premake
            -- automatically adds this for WindowedApp.
            links { "gdi32" }
end
function as_shared_lib()
            kind "SharedLib"
end

function lay_project(project_name, as_build_type, src_file)
    project(project_name)
        platforms { "x32", "x64" }
        -- kind "ConsoleApp"
        -- kind "WindowedApp"
        language "C"
        -- regular
        --files { "**.h", "**.c" }
        files { "**.h", src_file }

        if _ACTION == "vs2015" or _ACTION == "vs2017" then
            -- Build as C++ even with .c filename extension, because we
            -- need [] operator for our vector types (GCC provides this in
            -- C with vector_size special attribute, but need operator
            -- overload in C++)
            options { "ForceCpp" }
        end

        configuration {}
            flags { "ExtraWarnings" }
            targetdir(targetDir)
            as_build_type()

        configuration { "float" }
            defines { "LAY_FLOAT=1" }

        configuration { "vs*", "windows" }
            defines { "_CRT_SECURE_NO_WARNINGS" }
            buildoptions {
                -- Disable "unreferenced formal parameter" warning
                "-wd4100"
            }

        configuration { "gmake", "windows" }
            -- Nehalem chips were released in 2008, and include SSE4.
            --
            -- Note that gcc -Q --help=target -march=nehalem (or whatever other
            -- arch) will not correctly display the enabled/disabled flags for
            -- stuff like SSE, etc. This is a known problem with gcc (it's on
            -- their bug tracker).  -march=native, however, will show the
            -- enabled/disabled flags correctly.
            buildoptions {
                "-std=c99",
                "-Wno-unused-parameter",
                -- We want strict overflow on regardless of optimization level,
                -- because it will change program semantics.
                "-fstrict-overflow",
                "-fstrict-aliasing",
                "-Wstrict-aliasing=3",
                "-march=nehalem"
            }

        configuration "Debug or Develop"
            flags { "Symbols" }

        configuration { "Debug" }
            defines { "DEBUG" }
        configuration { "Debug", "gmake" }
            buildoptions {
                "-ggdb",
                "-fstack-protector-strong",
            }
            -- need to manually link -lssp for stack protector stuff
            links { "ssp" }
        configuration  { "Develop or Release", "gmake" }
            -- Remove some extra junk from the asm we don't need
            buildoptions {
                "-fno-stack-check",
                "-fno-dwarf2-cfi-asm",
                "-fno-asynchronous-unwind-tables",
            }

        configuration "Release"
            defines { "NDEBUG", "WIN32_LEAN_AND_MEAN", "VC_EXTRALEAN" }
            flags {
                "Optimize",
                "NoBufferSecurityCheck",
                "NoWinRT",
                "NoFramePointer",
            }
            windowstargetplatformversion "7"

        configuration { "Release", "gmake" }
            buildoptions {
                "-Ofast",
                "-fuse-linker-plugin",
                "-flto",
                "-fno-fat-lto-objects"
            }
            linkoptions {
                "-flto",
                "-fuse-linker-plugin",
                "-s"
            }
        -- SSE2 in msvc must be explicitly enabled when building 32-bit. In
        -- x64, /arch:SSE2 will produce a compiler warning, because it's
        -- redundant (SSE2 always available in x64).
        configuration { "Release or Develop", "x32" }
            flags { "EnableSSE2" }
        -- msvc builds are forced to C++, and we always want to disable
        -- exceptions and RTTI.
        configuration { "vs*" }
            flags { "NoExceptions", "NoRTTI" }
        configuration { "Release", "vs*" }
            flags { "FloatFast" }
end

solution "layout"
    location ( "build/" .. action )
    configurations { "Debug", "Develop", "Release" }
    lay_project("tests", as_console_app, "test_layout.c")
    lay_project("benchmark", as_console_app, "benchmark_layout.c")
    lay_project("luamodule", as_shared_lib, "luamodule_layout.c")
        incl_luajit()
        configuration {}
            targetname("layout")
