require "clean"
require "vscode"

workspace "Memory-Manager"

    location("Generated")
    cppdialect "C++23"
    warnings "Extra"
    fatalwarnings { "All" }
    configurations
    {
        "debug",
        "release"
    }

    architecture "x86_64"
    staticruntime "on"

    filter "configurations:debug"
        runtime "Debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:release"
        runtime "Release"
        defines { "NDEBUG" }
        optimize "On"

    rootPath = path.getdirectory(_SCRIPT)
    targetBuildPath = path.getdirectory(_SCRIPT) .. "/Build/target"
    objBuildPath = path.getdirectory(_SCRIPT) .. "/Build/obj"

    os.mkdir("Generated/Projects")

include "External"
include "Library"
include "Application"
include "Test"