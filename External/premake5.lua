local projectsPath = rootPath .. "/Generated/Projects"
local rtLib = ""

filter "configurations:release"
    rtLib = "\'MultiThreaded\'"

 filter "configurations:debug"
     rtLib = "\'MultiThreadedDebug\'"


project "GoogleTest"
    kind "StaticLib"
    location(projectsPath)

    targetdir(targetBuildPath .. "/External")
    objdir(objBuildPath .. "/%{prj.name}")

    libDirectory = "\"" .. path.getdirectory(_SCRIPT) .. "/%{prj.name}\""

    filter "system:windows"
        kind "Utility"

        prebuildcommands{
            "{MKDIR} %{prj.objdir}",
            "cmake -S " .. libDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DCMAKE_MSVC_RUNTIME_LIBRARY=" .. rtLib,
            "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
        }

    filter "system:linux"
        kind "Makefile"
        buildcommands{
            "{MKDIR} %{prj.objdir}",
            "cmake -S " .. libDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir}",
            "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
        }


project "SDL3"
    kind "StaticLib"
    location(projectsPath)

    moduleDirectory = "\"" .. path.getdirectory(_SCRIPT) .. "\"" .. "/%{prj.name}"

    targetdir(targetBuildPath .. "/External")
    objdir(objBuildPath .. "/%{prj.name}")

    filter "system:windows"
        kind "Utility"

		prebuildcommands{
			"{MKDIR} %{prj.objdir}",
			"cmake -S " .. moduleDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_LIBC=ON -DCMAKE_MSVC_RUNTIME_LIBRARY=" .. rtLib,
			"cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
		}

        --[[filter "configurations:debug"
            prebuildcommands{
                "{MKDIR} %{prj.objdir}",
                "cmake -S " .. moduleDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_LIBC=ON -DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreadedDebug'",
                "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
            }]]--
    filter "system:linux"
        kind "Makefile"
        buildcommands{
            "{MKDIR} %{prj.objdir}",
            "cmake -S " .. moduleDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_LIBC=ON -DSDL_INSTALL=ON",
            "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
        }

project "ImGui"
    kind "StaticLib"
    location(projectsPath)
    dependson("SDL3")

    warnings "Off"

    targetdir(targetBuildPath .. "/External/lib/")
    objdir(objBuildPath .. "/%{prj.name}")


    files {
        "ImGui/imgui*.cpp",
        "ImGui/backends/imgui_impl_sdlrenderer3.cpp",
        "ImGui/backends/imgui_impl_sdl3.cpp"
    }

    includedirs{
        "ImGui/",
        "ImGui/backends/",
        targetBuildPath .. "/External/include/"
    }

    mkdirPath = "\"" .. targetBuildPath .. "/External/include/%{prj.name}\""
    copyPath = "\"" .. targetBuildPath .. "/External/include/%{prj.name}\""

    prebuildcommands{
        "{MKDIR} " .. mkdirPath,
        "{COPY} " .. rootPath .. "/External/ImGui/*.h " .. copyPath,
        "{COPY} " .. rootPath .. "/External/ImGui/backends/imgui_impl_sdlrenderer3.h " .. copyPath,
        "{COPY} " .. rootPath .. "/External/ImGui/backends/imgui_impl_sdl3.h " .. copyPath
    }

project "TracyClient"
    kind "StaticLib"
    location(projectsPath)

    warnings "off"

    targetdir(targetBuildPath .. "/External/lib/")
    objdir(objBuildPath .. "/TracyClient")

    libDirectory = "\"" .. path.getdirectory(_SCRIPT) .. "/Tracy\""
	
    files {
        "Tracy/public/TracyClient.cpp",
    }

	defines{ "TRACY_ENABLE", "TRACY_DETAILED" }

    includedirs{
        "Tracy/server/",
        "Tracy/public/",
        "Tracy/public/client/",
        "Tracy/public/common/",
        "Tracy/public/libbacktrace/",
        "Tracy/public/tracy/",
        targetBuildPath .. "/External/include/"
    }

    mkdirPath = "\"" .. targetBuildPath .. "/External/include/%{prj.name}\""
    copyPath = "\"" .. targetBuildPath .. "/External/include/%{prj.name}\""

    prebuildcommands{
        "{MKDIR} " .. mkdirPath,
        "{COPY} " .. rootPath .. "/External/Tracy/server/*.h* " .. copyPath .. "/server",
        "{COPY} " .. rootPath .. "/External/Tracy/public/client/*.h* " .. copyPath .. "/public/client",
        "{COPY} " .. rootPath .. "/External/Tracy/public/common/*.h* " .. copyPath .. "/public/common",
        "{COPY} " .. rootPath .. "/External/Tracy/public/libbacktrace/*.h* " .. copyPath .. "/public/libbacktrace",
        "{COPY} " .. rootPath .. "/External/Tracy/public/tracy/*.h* " .. copyPath .. "/public/tracy",
    }

project "TracyServer"

    kind "StaticLib"
    location(projectsPath)

    targetdir(targetBuildPath .. "/External")
    objdir(objBuildPath .. "/TracyServer")

    libDirectory = "\"" .. path.getdirectory(_SCRIPT) .. "/Tracy\""

	defines{ "TRACY_ENABLE", "TRACY_DETAILED" }

    filter "system:windows"
        kind "Utility"

        prebuildcommands{
            --[[
			"{MKDIR} %{prj.objdir}",
            "cmake -S " .. libDirectory .. "/profiler -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DCMAKE_MSVC_RUNTIME_LIBRARY=" .. rtLib,
            "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
			--]]
        }

    filter "system:linux"
        kind "Makefile"
        buildcommands{
            "{MKDIR} %{prj.objdir}",
            "cmake -S " .. libDirectory .. "/profiler -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir}",
            "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
        }
