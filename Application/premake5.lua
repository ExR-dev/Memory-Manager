project "Application"

    kind "ConsoleApp"
    location(rootPath .. "/Generated")

    local externalLibPath = targetBuildPath .. "/External/lib"
    libdirs(externalLibPath)
    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")

    files {rootPath .. "/Application/inc/**.hpp", rootPath .. "/Application/src/**.cpp"}

    includedirs{"../Library/include", targetBuildPath .. "/External/include" , "inc"}
    dependson{"ImGui", "SDL3"}

    links{"Library", "ImGui"}

    filter "system:windows"
        links{"SDL3-static", "imagehlp", "setupapi", "user32", "version", "uuid", "winmm", "imm32"}       

    filter "system:linux"
        links{externalLibPath .. "/SDL3"}
