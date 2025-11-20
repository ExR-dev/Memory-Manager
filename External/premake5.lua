project "GoogleTest"

    kind "StaticLib"
    location(rootPath .. "/Generated/Projects")

    targetdir(targetBuildPath .. "/External")
    objdir(objBuildPath .. "/%{prj.name}")

    libDirectory = "\"" .. path.getdirectory(_SCRIPT) .. "\"" .. "/%{prj.name}"

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
		
project "Tracy"

    kind "StaticLib"
    location(rootPath .. "/Generated/Projects")

    targetdir(targetBuildPath .. "/External")
    objdir(objBuildPath .. "/%{prj.name}")

    libDirectory = "\"" .. path.getdirectory(_SCRIPT) .. "\"" .. "/%{prj.name}"

    filter "system:windows"
        kind "Utility"
        prebuildcommands{
            "{MKDIR} %{prj.objdir}",
            "cmake -S " .. libDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreadedDebug' -DTRACY_ENABLE=ON -DTRACY_ON_DEMAND=ON",
            "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
        }

    filter "system:linux"
        kind "Makefile"
        buildcommands{
            "{MKDIR} %{prj.objdir}",
            "cmake -S " .. libDirectory .. " -B %{prj.objdir} -DCMAKE_INSTALL_PREFIX=%{prj.targetdir} -DTRACY_ENABLE=ON -DTRACY_ON_DEMAND=ON",
            "cmake --build %{prj.objdir} --config %{cfg.buildcfg} --target install",
        }