project "Application"

    kind "ConsoleApp"
    location(rootPath .. "/Generated/Projects")

    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {"src/**.hpp", "src/**.cpp"}
    includedirs{"../Library/include"}

    links{"Library"}