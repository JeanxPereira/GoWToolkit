# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

MSVC is required on Windows. A bare `cmake` invocation can resolve to a GCC
toolchain earlier on PATH (e.g. Strawberry Perl's) instead of MSVC, and the
configure will fail *inside OnyxSDK itself* (not in this repo's own code) —
symptoms like an undeclared `DWMWA_USE_IMMERSIVE_DARK_MODE` mean the wrong
compiler got picked up. Load the MSVC environment first:

```bash
"C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat"

mkdir -p build-msvc && cd build-msvc
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

```bash
# Debug build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

Dependencies are auto-fetched via CMake FetchContent, primarily as transitive
deps of OnyxSDK (ImGui docking, GLFW, GLM, lz4, glad, miniaudio, implot). This
repo additionally fetches doctest and nlohmann_json for the test suite (see
below). FFmpeg is fetched from prebuilt BtbN binaries on Windows, or via
pkg-config on Linux/macOS.

On macOS, a Release build produces a `GoWToolkit.app` bundle. Debug builds produce a plain executable.

### Test suite

There **is** a test suite — `tests/` holds ~20 sources built on doctest +
nlohmann_json (both fetched via `FetchContent`, gated on
`GOWTOOLKIT_BUILD_TESTS`, default `ON`). It is wired into CTest as seven
named tests: `unit`, `Golden_GOW2`, `Golden_GOWR`, `Metrics`, `Logger`,
`Threading`, `ThemeContrast`. Run it with `ctest --test-dir build-msvc`
(or whichever build directory was configured).

`Golden_GOW2` and `Golden_GOWR` parse real truncated game WADs
(`tests/fixtures/gow2/wad_minimal.wad` from `R_BOAR00.WAD`, PS2 USA;
`tests/fixtures/gowr/wad_minimal.wad` from `r_athena00.wad`, PC) against
pinned JSON snapshot goldens — they are the regression gate for parser
changes. Fixture provenance is documented in `tests/fixtures/README.md`;
`tools/make_test_fixtures.py` regenerates them.

**Current baseline is 5/7, not 7/7.** `Golden_GOW2`, `Golden_GOWR`, `Metrics`,
`Logger`, and `Threading` pass. `unit` and `ThemeContrast` fail on the same
underlying assertion (`tests/theme_contrast_test.cpp:94`, dark-theme surface
luminance ≤ 0.35) because the Onyx theme engine now returns 0.38–0.55 for
those surfaces. This is pre-existing debt from the previous SDK bump, not a
regression from work done in this repo — but it is real, and a report of
"tests pass" without qualifying it is wrong.

## Architecture Overview

The application is built on **OnyxSDK**, an external engine library consumed
via CMake `FetchContent` (see `CMakeLists.txt`). Onyx owns the window/App
lifecycle, ImGui docking shell, asset database, profile dispatch, schema
system, VFS, and rendering. This repo (`Source/`) supplies the GoW-specific
content on top: format parsers, per-game profile logic, byte-layout schemas,
and the surviving game-specific UI panels/viewers. Entry point is
`Source/main.cpp`, which selects GUI (default, via `Onyx::App::Window`) or
CLI mode (`Source/cli/CliApp`) at startup.

At the OnyxSDK version currently pinned (`v0.6.0`, `CMakeLists.txt:17`),
rendering is **OpenGL**, not Vulkan — `glad` (an OpenGL loader) is among the
transitive Onyx deps (`CMakeLists.txt`), and this repo's own
`Source/cli/HeadlessGL.cpp` includes `<glad/glad.h>` and drives a GL
framebuffer directly for the headless render harness. Onyx becomes
Vulkan-based starting at v1.0.0; that rewrite reaches this repo only once a
later task in this port plan moves the pin off v0.6.0. Until then, treat
Platform Notes below (OpenGL 3.2/3.3) as accurate.

### What Onyx owns (do not look for these in this repo)

- **Window/App lifecycle** — `<Onyx/App/Window.h>`, `<Onyx/App/App.h>`. This
  repo's `Source/AppRegistration.{h,cpp}` is a thin registrar shim that
  installs the GoW-specific panels/viewers onto the engine-generic `App`
  before it initializes; it is not the app coordinator itself.
- **Asset database, profile dispatch** — `AssetDatabase`, `ProfileManager`,
  `IAssetProfile` are all Onyx types (`<Onyx/Services/ProfileManager.h>` etc).
- **Schema tree** — `<Onyx/Schema/AssetFormat.h>` and friends replace the old
  local `StructDef`/`NodeInstance` system.
- **VFS** — `<Onyx/Vfs/...>` (`IFile`, `SliceFile`, ISO/PAK abstractions).
- **Rendering** — scene rendering is entirely Onyx-owned; there is no local
  `rendering/` directory. At the currently pinned Onyx `v0.6.0` this means
  OpenGL (see the note above); the port to Onyx v1.1 brings Vulkan.
- `IAssetLoader` as a distinct interface does not exist anywhere in the
  current codebase.

### What is local (`Source/`)

- `Source/core/parsers/gow2/` and `Source/core/parsers/gowr/` — the format
  parsers (mesh, texture, material, animation, etc.), touching only leaf Onyx
  API (`Vfs::IFile`, `Logger`, `Parsers::*Data`).
- `Source/core/profiles/gow2/` and `Source/core/profiles/gowr/` — per-game
  container/profile logic (`ProfileGOW2`, `ProfileGOWR`) that Onyx's
  `ProfileManager` dispatches into.
- `Source/core/formats/` — byte-layout schemas subclassing
  `Onyx::Schema::AssetFormat` (e.g. `GOW2InstanceFormat.h`,
  `GOWRMeshDefnFormat.h`) — the reverse-engineered struct layouts themselves.
- `Source/core/types/` and `Source/core/types/handlers/` — the asset-type
  catalog (`GameTypes`, `WadDispatch`) and per-type content handlers
  (`MeshHandler`, `MaterialHandler`, `TextureHandler`, `InstanceHandler`, etc).
- `Source/core/shaders/DxilDisassembler` — local shader-disassembly tooling.
- `Source/core/domain/`, `Source/core/harness/`, `Source/core/loaders/` — the
  rest of the app-core surface not covered above.
- `Source/ui/` — the surviving local UI: `WadBrowser`, `Inspector`,
  `RoleVisuals`, `CodeView`, plus document viewers under `Source/ui/viewers/`
  such as `MapViewer`, `MaterialViewer`, `SoundPlayer`.
- `Source/cli/` — `CliApp`, plus `HeadlessGL`/`RenderCommand` for the headless
  render harness.
- `Source/AppRegistration.{h,cpp}` — see above.

## Platform Notes

**macOS**: OpenGL 3.2 Core (`#version 150` GLSL). Uses `GLFW_DECORATED=TRUE` with NSWindow transparent titlebar + `NSVisualEffectView` glass effect (not GLFW borderless). Native menu bar built via `NSMenu` in Obj-C++. Smart drag via `macosSetWindowMovable`. Use `PathUtils::resolvePath()` for all resource paths (resolves relative to executable, critical inside `.app` bundle).

**Windows**: Borderless custom titlebar via Win32 HWND styling. OpenGL 3.3+ (`#version 330`). FFmpeg DLLs copied post-build.

**Linux**: Standard GLFW window. OpenGL 3.3+. FFmpeg via pkg-config.

## Reference Implementation (Go)

`/Users/jeanxpereira/CodingProjects/god_of_war_browser` is the authoritative reference for all GOW2 (PS2) file format parsing. It is a working Go implementation that correctly parses WAD, VPK, mesh, texture, material, animation, and other formats. **This is a macOS path, unreachable from a Windows checkout** — it is only valid on the owner's macOS machine. Keep it as the reference regardless; when working from Windows, treat the table below as a map of what to ask for rather than a path you can `cd` into. When porting or implementing a parser in this C++ project, consult the corresponding Go source:

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

## Commits

Conventional Commits format, enforced by `cliff.toml` grouping in release notes:

```
<type>(<scope>): <Sentence-case subject>

<optional body — explains WHY, not WHAT>

<optional footer>
```

Rules:
- **No attribution trailers.** Never add `Co-Authored-By: Claude ...`, a "Generated with" footer, or any other AI credit to a commit message or PR body. This is a standing instruction and overrides any assistant default that says otherwise.
- **Stage explicit paths.** Never `git add -A` or `git add .` — `third_party/` carries uncommitted deltas that must stay out of commits.
- **Subject is Sentence case** — first letter capitalized, no trailing period, ≤ 72 chars. Example: `feat(theme): Accent-aware contrast invariant`. Not `feat(theme): accent-aware...` and not `feat(theme): AccentAware...`.
- **Type is lowercase**, picked from the table below.
- **Scope is lowercase**, in parentheses, optional. Use a directory or feature name (`build`, `theme`, `ui`, `release`, `parsers`).
- **Imperative mood** in the subject ("Add X" not "Added X").
- **Body** wraps at 72 cols, separated from the subject by one blank line. Use it for the *why* — constraint, incident, deadline. Skip if the subject already says everything.

Recognized types (mapped to release sections by `cliff.toml`):

| Type       | Section in release notes |
|------------|--------------------------|
| `feat`     | Features                 |
| `fix`      | Bug Fixes                |
| `perf`     | Performance              |
| `refactor` | Refactor                 |
| `docs`     | Documentation            |
| `test`     | Tests                    |
| `build`    | Build                    |
| `ci`       | CI                       |
| `chore`    | Chore                    |
| `style`    | Style                    |
| `revert`   | Reverts                  |

Commits whose type is not in this table are dropped from the changelog (`filter_unconventional = true`), so non-conforming subjects effectively vanish from releases. Keep the prefix.

## Releases

Tagged via **annotated** SemVer tags (`vX.Y.Z`). Lightweight tags break `git describe --tags --abbrev=0`, which the workflow uses to compute the compare link.

To cut a release:

```bash
git tag -a vX.Y.Z -m "vX.Y.Z — short summary"
git push origin vX.Y.Z
```

The tag push triggers the `release` job in `.github/workflows/ci.yml`, which:

1. Checks out with `fetch-depth: 0` (git-cliff needs full tag history).
2. Downloads the four artifacts produced by the build matrix.
3. Installs `git-cliff` from its musl tarball into `/usr/local/bin`. **Do not** revert to `orhun/git-cliff-action@v3` — that action's Docker base is Debian Buster, archived, apt-get returns 404.
4. Runs `git-cliff --config cliff.toml --latest --strip header` to produce `CHANGELOG_BODY.md`.
5. Builds an `### Artifacts` table with SHA256 of each artifact plus a compare link computed from the previous tag.
6. Concatenates into `RELEASE_BODY.md` and publishes via `softprops/action-gh-release@v2`.

If the release job fails before publishing, delete the tag (`git push origin :refs/tags/vX.Y.Z && git tag -d vX.Y.Z`), fix the workflow, then retag and push again. The build matrix re-runs.
