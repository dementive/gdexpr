#!/usr/bin/env python

# ignore missing method errors...if you are reading this and know how to actually import Glob and Default from SCons please help me fix this...
# ruff: noqa: F821

from glob import glob
from pathlib import Path
from SCons.Script import SConscript

env = SConscript("godot-cpp/SConstruct")

# Add source files.
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")  # type: ignore

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData(
            "src/gen/doc_data.gen.cpp",
            source=Glob("doc_classes/*.xml"),  # type: ignore
        )
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

# Find gdextension path even if the directory or extension is renamed (e.g. project/addons/gdexpr/gdexpr.gdextension).
(extension_path,) = glob("project/addons/*/*.gdextension")

# Find the addon path (e.g. project/addons/gdexpr).
addon_path = Path(extension_path).parent

# Find the project name from the gdextension file (e.g. gdexpr).
project_name = Path(extension_path).stem

# Create the library target (e.g. libgdexpr.linux.debug.x86_64.so).
debug_or_release = "release" if env["target"] == "template_release" else "debug"
if env["platform"] == "macos":
    library = env.SharedLibrary(
        "{0}/bin/lib{1}.{2}.{3}.framework/{1}.{2}.{3}".format(
            addon_path,
            project_name,
            env["platform"],
            debug_or_release,
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "{}/bin/lib{}.{}.{}.{}{}".format(
            addon_path,
            project_name,
            env["platform"],
            debug_or_release,
            env["arch"],
            env["SHLIBSUFFIX"],
        ),
        source=sources,
    )

Default(library)  # type: ignore
