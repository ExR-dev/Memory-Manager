project "Library"

    kind "StaticLib"
    location(rootPath .. "/Generated")

    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {rootPath .. "/Library/include/**.hpp", rootPath .. "/Library/src/**.cpp"}
    includedirs{"include/"}