project "Application"

    kind "ConsoleApp"
    location(rootPath .. "/Application")

    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {"inc/**.hpp", "src/**.cpp"}
    includedirs{"../Library/include", targetBuildPath .. "/External/include" , "inc"}
    dependson{"ImGui", "SDL3"}

    local externalLibPath = targetBuildPath .. "/External/lib"
    links{"Library",
        externalLibPath .. "/ImGui",
        externalLibPath .. "/SDL3"}
