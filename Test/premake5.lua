project "Test"

    kind "ConsoleApp"
    location(rootPath .. "/Generated/Projects")

    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {"src/**.h", "src/**.cpp"}
    includedirs{"../Library/include", targetBuildPath .. "/External/include"}

    libdirs{targetBuildPath .. "/External/lib"}

    dependson {"GoogleTest", "Library"}
    links{"Library", "gtest"}