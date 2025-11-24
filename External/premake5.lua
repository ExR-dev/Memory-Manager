local projectsPath = rootPath .. "/Generated/Projects"

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
            "cmake -S " .. libDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreadedDebug'",
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

        filter "configurations:release"
            prebuildcommands{
                "{MKDIR} %{prj.objdir}",
                "cmake -S " .. moduleDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_LIBC=ON -DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreaded'",
                "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
            }

        filter "configurations:debug"
            prebuildcommands{
                "{MKDIR} %{prj.objdir}",
                "cmake -S " .. moduleDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_LIBC=ON -DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreadedDebug'",
                "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
            }
    filter "system:linux"
        kind "Makefile"
        buildcommands{
            "{MKDIR} %{prj.objdir}",
            "cmake -S " .. moduleDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_LIBC=ON -DSDL_INSTALL=OFF",
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