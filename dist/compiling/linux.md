### Compiling GoWToolkit on Linux

GoWToolkit is built with GCC or Clang. CMake 3.20+ is required.

#### Prerequisites

Install the build tools and OpenGL/X11 development headers.

**Debian / Ubuntu:**

```sh
sudo apt install cmake ninja-build g++ libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev
```

**Fedora:**

```sh
sudo dnf install cmake ninja-build gcc-c++ mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel wayland-devel libxkbcommon-devel
```

**Arch Linux:**

```sh
sudo pacman -S cmake ninja gcc mesa libx11 libxrandr libxinerama libxcursor libxi wayland libxkbcommon
```

#### Build

```sh
cd GoWToolkit
mkdir -p build
cd build
cmake -G "Ninja"                \
  -DCMAKE_BUILD_TYPE=Release    \
  ..
ninja
```

The executable will be at `build/GoWToolkit`.

#### Notes

- All libraries (ImGui, GLFW, GLM, lz4, glad, miniaudio) are fetched automatically by CMake via `FetchContent`.
- GLFW requires X11 or Wayland development headers to be installed on the system.
- The build links `GL`, `dl`, `pthread`, and `m` system libraries automatically.
- Fonts are copied to the output directory as a post-build step.
