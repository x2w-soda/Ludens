## Build Instructions

Ludens engine runtime, builder, and editor are supported on Windows and Linux.

- CMake is used to generate build system files.
- Most dependencies are git-cloned via CMake `FetchContent` during configuration time.
- Some dependencies may be platform-specific, see below.
- Main CMake executable targets are `LDEditor`, `LDBuilder`, and `LDRuntime`.

To build the codebase, clone the repository and submodules on your platform. Currently a git submodule [LudensLFS](https://github.com/x2w-soda/LudensLFS) is used for storing larger asset files and test suites.

```
git clone --recursive https://github.com/x2w-soda/Ludens
git submodule init
git submodule update
```

### Building on Windows

The official [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) is required, versions 1.3.268 and higher should suffice. The Vulkan SDK also bundles shader compiler dependencies `glslang` and `spirv-cross` as well.

Generate build system files with CMake, missing dependencies should be detected by CMake. I use Visual Studio for development on Windows.

```
mkdir build
cd build
cmake ..
```

Switch between debug/release builds in Visual Studio directly.

### Building on Linux

Most dependencies could be found as official packages by your package manager, but `Vulkan` and `spirv-cross` may be specific to your GPU vendor as well. `spirv-cross` is usually not found as an official package and may appear as community-provided packages (AUR package for Arch-based or Copr package for Fedora). If `spirv-cross` still could not be found, you will have to build from [source](https://github.com/KhronosGroup/SPIRV-Cross) via the official Khronos repositories.

Generate build system files with CMake, missing dependencies should be detected by CMake. I use Ninja on Linux but GNU Make should also work out of the box.

```
mkdir build
cd build
cmake ..
```

To later switch to debug or release builds, configure the `CMAKE_BUILD_TYPE` variable.

```
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

or

```
cmake .. -DCMAKE_BUILD_TYPE=Release
```