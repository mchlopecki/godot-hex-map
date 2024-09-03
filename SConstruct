#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")



# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")
sources += Glob("src/hex_map/*.cpp")
sources += Glob("src/hex_map/editor/*.cpp")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libhexmap.{}.{}.framework/libhexmap.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "demo/bin/libhexmap{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)

# ANNOYING
# * by default, godot-cpp scons makes shared library symbol visibility to hidden
# * by default linking allows for undefined symbols that just panic
#
# So when we built the dll, the symbols we're testing were hidden.  When we
# linked the binary, the linker couldn't find the symbols in the shared library,
# but instead of erroring, it redirects those to an __abort_with_payload() call.
# So the binary links, you execut it and it crashes.
#
# This shitty workaround, just recompile the sources and link them directly into
# the test binary.

godot_cpp = File(f"godot-cpp/bin/libgodot-cpp{env["suffix"]}.a")
tests = env.Program(
    target='tests/run_tests',
    source=Glob("tests/*cpp") + sources,
    LIBS=[godot_cpp]
)

# import pdb
# pdb.set_trace()

run_tests = Command("run_tests", None, "tests/run_tests" )
Depends(run_tests, tests)
