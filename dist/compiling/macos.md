### Compiling GoWToolkit on macOS

GoWToolkit is built with the system Clang (Apple Clang) or LLVM Clang. CMake 3.20+ is required.

#### Prerequisites

Install the build tools via [Homebrew](https://brew.sh/):

```sh
brew install cmake ninja
```

No other dependencies are needed. All libraries (ImGui, GLFW, GLM, lz4, glad, miniaudio) are fetched automatically by CMake via `FetchContent`.

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

#### Build (Xcode)

To generate an Xcode project instead:

```sh
cd GoWToolkit
mkdir -p build
cd build
cmake -G "Xcode" ..
open GoWToolkit.xcodeproj
```

#### Debug build

```sh
cmake -G "Ninja"                \
  -DCMAKE_BUILD_TYPE=Debug      \
  ..
ninja
```

#### Notes

- macOS uses OpenGL 3.2 Core Profile (`#version 150` GLSL). Apple deprecated OpenGL but it still works.
- The build automatically links Cocoa, OpenGL, AudioToolbox, CoreAudio, IOKit, and CoreFoundation frameworks.
- Fonts are copied to `build/third_party/fonts/` as a post-build step.
