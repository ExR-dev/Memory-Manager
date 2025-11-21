project "Application"

    kind "ConsoleApp"
    location(rootPath .. "/Application")

    local externalLibPath = targetBuildPath .. "/External/lib"
    libdirs(externalLibPath)
    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {"inc/**.hpp", "src/**.cpp"}
    includedirs{"../Library/include", targetBuildPath .. "/External/include" , "inc"}
    dependson{"ImGui", "SDL3"}

    links{"Library",
        "/ImGui",
        "/SDL3"}
