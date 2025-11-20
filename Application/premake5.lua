project "Application"

    kind "ConsoleApp"
    location(rootPath .. "/Application")

    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {"inc/**.hpp", "src/**.cpp"}
    includedirs{"../Library/include", "inc"}

    links{"Library"}