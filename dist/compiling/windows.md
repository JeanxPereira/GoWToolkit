### Compiling GoWToolkit on Windows

GoWToolkit can be built with MSVC (Visual Studio) or MinGW. CMake 3.20+ is required.

#### Option A: Visual Studio (MSVC)

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the "Desktop development with C++" workload.
2. Install [CMake](https://cmake.org/download/) (or use the one bundled with Visual Studio).
3. Build:

```cmd
cd GoWToolkit
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

The executable will be at `build\Release\GoWToolkit.exe`.

#### Option B: MSYS2 / MinGW

1. Install [MSYS2](https://www.msys2.org/).
2. Open the `MSYS2 MinGW x64` shell.
3. Install dependencies:

```sh
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-gcc
```

4. Build:

```sh
cd GoWToolkit
mkdir -p build
cd build
cmake -G "Ninja"                \
  -DCMAKE_BUILD_TYPE=Release    \
  ..
ninja
```

#### Notes

- All libraries (ImGui, GLFW, GLM, lz4, glad, miniaudio) are fetched automatically by CMake via `FetchContent`.
- The build links `opengl32`, `comdlg32`, and `dwmapi` system libraries automatically.
- In Release mode, the executable is built as a Win32 application (no console window).
- Fonts are copied to the output directory as a post-build step.
