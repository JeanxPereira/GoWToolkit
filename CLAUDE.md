# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Standard build
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja

# Debug build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

Dependencies are auto-fetched via CMake FetchContent (ImGui docking, GLFW 3.3.9, GLM 1.0.1, lz4 1.9.4). FFmpeg is fetched from prebuilt BtbN binaries on Windows, or via pkg-config on Linux/macOS.

On macOS, a Release build produces a `GoWToolkit.app` bundle. Debug builds produce a plain executable.

There is no test suite — testing is done manually via the GUI or CLI mode.

## Architecture Overview

The application has two entry modes, selected at startup in `src/main.cpp`: GUI (default) and CLI (`src/cli/CliApp`).

### Layer Stack

**Window** (`src/window/`) — GLFW + ImGui lifecycle, per-platform setup (`.cpp` Win/Linux, `.mm` macOS). Runs the frame loop and calls into App.

**App** (`src/App.h/cpp`) — Main UI coordinator. Manages the ImGui DockSpace, panel registration, menu bar, and config persistence.

**UI** (`src/ui/`) — Panels (IsoBrowser, PakBrowser, WadBrowser, Inspector, StatusBar, SettingsWindow) and document viewers (Viewport3D, ImageViewer, MaterialViewer, SoundPlayer, VideoPlayer, MapViewer). Panels are registered via `PanelRegistry`; viewers via `ViewerRegistry`. Each panel receives an `AppContext` reference for cross-panel communication.

**Core** (`src/core/`) — All game-format logic, no UI dependency:
- `AssetDatabase`: loads WADs/ISOs, manages async loading state
- `ProfileManager` + `IGameProfile`: game variant detection and dispatch (GOW2, GOWR implementations)
- `IAssetLoader` / loaders: per-asset-type parsing (mesh, texture, material, sound)
- `schema/`: `StructDef` + `NodeInstance` tree representing parsed game data
- `vfs/`: virtual filesystem abstractions (`IsoFileSystem`, PAK support)
- `AppConfig`: binary config format "GTKC" v2 persisted to `gowtool.cfg`

**Rendering** (`src/rendering/`) — OpenGL scene rendering: `SceneRenderer`, `GpuMesh`, `Camera`, `GridRenderer`, `ShaderManager`.

### Data Flow

1. User opens ISO/PAK → `AssetDatabase::LoadWadAsync` → `ProfileManager` detects game → `IGameProfile::ParseWad` → populates database
2. Each frame: `App` iterates panels → panels query `AppContext` for loaded assets → active `DocumentWindow` renders its `IDocumentContent` viewer
3. Config is loaded at startup and persisted on exit via `AppConfig`

### Key Interfaces

- `IGameProfile` — implemented by `ProfileGOW2` and `ProfileGOWR`
- `IAssetLoader` — one per asset type per game
- `IPanel` — all dockable panels
- `IDocumentContent` — all document viewers
- `IVirtualFileSystem` / `IFile` — filesystem abstraction over ISO and PAK

## Platform Notes

**macOS**: OpenGL 3.2 Core (`#version 150` GLSL). Uses `GLFW_DECORATED=TRUE` with NSWindow transparent titlebar + `NSVisualEffectView` glass effect (not GLFW borderless). Native menu bar built via `NSMenu` in Obj-C++. Smart drag via `macosSetWindowMovable`. Use `PathUtils::resolvePath()` for all resource paths (resolves relative to executable, critical inside `.app` bundle).

**Windows**: Borderless custom titlebar via Win32 HWND styling. OpenGL 3.3+ (`#version 330`). FFmpeg DLLs copied post-build.

**Linux**: Standard GLFW window. OpenGL 3.3+. FFmpeg via pkg-config.

## Reference Implementation (Go)

`/Users/jeanxpereira/CodingProjects/god_of_war_browser` is the authoritative reference for all GOW2 (PS2) file format parsing. It is a working Go implementation that correctly parses WAD, VPK, mesh, texture, material, animation, and other formats. When porting or implementing a parser in this C++ project, consult the corresponding Go source:

| Format | Go reference path |
|--------|------------------|
| WAD structure | `pack/wad/wad.go`, `pack/wad/gow2.go` |
| Mesh / MDL | `pack/wad/mesh/`, `pack/wad/mdl/` |
| Texture (TXR) | `pack/wad/txr/` |
| Material (MAT) | `pack/wad/mat/` |
| Object / Instance | `pack/wad/obj/`, `pack/wad/inst/` |
| Animation | `pack/wad/anm/` |
| Sound (SBK/VAG) | `pack/wad/sbk/`, `pack/vag/`, `pack/adpcm/` |
| VPK | `pack/vpk/vpk.go` |
| Script | `pack/wad/scr/` |
| ISO / PAK (VFS) | `vfs/` |
| PS2 GPU / VIF | `ps2/vif/`, `ps2/dma/` |

The Go project uses a web UI (`web/`) and serves parsed data as JSON — the handler files (`webhandlers.go` inside each format dir) show exactly what fields are extracted and how they map to the binary layout.
