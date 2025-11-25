project "Test"

    kind "ConsoleApp"
    location(rootPath .. "/Generated")

    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {rootPath .. "/Test/src/**.h", rootPath .. "/Test/src/**.cpp"}
    includedirs{"../Library/include", targetBuildPath .. "/External/include"}

    libdirs{targetBuildPath .. "/External/lib"}

    dependson {"GoogleTest", "Library", "GoogleBenchmark", "TracyClient", "TracyServer"}
    links{"Library", "gtest", "benchmark", "tracy"}