# Building GoWToolkit

GoWToolkit uses CMake 3.20+ and C++20. All third-party libraries are fetched automatically during configuration — no manual dependency management needed.

## Quick Start

```sh
mkdir -p build
cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
ninja
```

## Platform Guides

- [macOS](dist/compiling/macos.md)
- [Windows](dist/compiling/windows.md)
- [Linux](dist/compiling/linux.md)

## Dependencies

All fetched automatically via CMake `FetchContent`:

| Library    | Version  | Purpose                    |
|------------|----------|----------------------------|
| ImGui      | docking  | UI framework               |
| GLFW       | 3.3.9    | Window/input management    |
| GLM        | 1.0.1    | Math (vectors, matrices)   |
| lz4        | 1.9.4    | Decompression              |
| glad       | bundled  | OpenGL loader              |
| miniaudio  | bundled  | Audio playback             |

## Build Types

| Type      | Flags           | Use case         |
|-----------|-----------------|------------------|
| `Debug`   | `-O0 -g`        | Development      |
| `Release` | `-O2 -DNDEBUG`  | Distribution     |
| `RelWithDebInfo` | `-O2 -g` | Profiling       |

## Troubleshooting

**CMake can't find OpenGL:**
Install your platform's OpenGL development headers (see platform guides).

**FetchContent download fails:**
Check your internet connection. CMake downloads ImGui, GLFW, GLM, and lz4 during the first configuration.

**Fonts missing at runtime:**
The build copies `third_party/fonts/` to the output directory automatically. If running from a non-standard location, ensure the fonts directory is next to the executable.
