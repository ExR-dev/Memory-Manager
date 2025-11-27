project "Test"

    kind "ConsoleApp"
    location(rootPath .. "/Generated")

    targetdir(targetBuildPath .. "/%{prj.name}")
    objdir(objBuildPath .. "/%{prj.name}")
    files {rootPath .. "/Test/src/**.h", rootPath .. "/Test/src/**.cpp"}
    includedirs{"../Library/include", targetBuildPath .. "/External/include"}

    libdirs{targetBuildPath .. "/External/lib"}
	
	removedefines "TRACY_ENABLE"
	removedefines "TRACY_DETAILED"

    dependson {"GoogleTest", "Library", "TracyClient"}
    links{"Library", "gtest"}