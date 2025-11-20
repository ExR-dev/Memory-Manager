project "Library"

    kind "StaticLib"
    location(rootPath .. "/Library")

    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {"include/**.hpp", "src/**.cpp"}
    includedirs{"include/"}