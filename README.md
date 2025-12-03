# DV2591 - Assignment 2

## Prerequisites

* **CMake** - https://cmake.org/download/
* **Premake5** - https://premake.github.io/download/
* **Visual Studio 2022 (Windows only)** - https://visualstudio.microsoft.com/downloads/ 

## Building the project

### Linux

1. `cd $(root-dir)`
2. `premake5 gmake`
3. `cd ./Generated`
4. `make`

### Windows

1. `cd $(root-dir)`
2. `premake5 vs2022`
3. Open solution in `./Generated`
4. Compile and build in Visual Studio

## Troubleshooting

* **The build fails due to a path missing (i.e. "gtest/gtest.h" is not found)**
  * You most likely have not cloned the dependent submodules. This can be resolved by executing `git submodule update --init --remote --recursive` in the root directory of the project

* **Premake5 is not recognized**
  * You have not added premake5 to the PATH variable (windows) or `/bin/` (linux). This can be resolved by adding the binary executable to the root directory or PATH or `/bin`

* **Build fails for any other reason**
  * Make sure all prerequisites are installed

* **If the issue persists**
  * Contact us through mail