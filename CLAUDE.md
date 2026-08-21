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
deps of OnyxSDK (ImGui docking, GLFW, GLM, lz4, volk, VMA, miniaudio, implot). This
repo additionally fetches doctest and nlohmann_json for the test suite (see
below). FFmpeg is fetched from prebuilt BtbN binaries on Windows, or via
pkg-config on Linux/macOS.

On macOS, a Release build produces a `GoWToolkit.app` bundle. Debug builds produce a plain executable.

### Test suite

There **is** a test suite — `tests/` holds ~20 sources built on doctest +
nlohmann_json (both fetched via `FetchContent`, gated on
`GOWTOOLKIT_BUILD_TESTS`, default `ON`). Run it with
`ctest --test-dir build-msvc` (or whichever build directory was configured).

CTest lists **19** entries, and only the first 11 are this repo's: `unit`,
`Golden_GOW2`, `Golden_GOWR`, `Metrics`, `Threading`, `ThemeContrast`,
`GowrClassify`, `Gow2Material`, `Logger`, `Selection`, `Visibility`. The
remaining 8 (`VkBootSmoke`, `VkSceneSmoke`, `RenderToImageSmoke`,
`VkAnimation`, `OracleReproducible`, `VkOracleParity`, `ColdStart`,
`VkValidationSelfTest`) belong to OnyxSDK, which registers them even though
this build sets `ONYX_BUILD_TESTS=OFF` -- a known SDK bug, harmless here
because they happen to build and pass. `VkValidationSelfTest` reports
Skipped without the Vulkan validation layer installed; that is an
environment fact, not a failure.

`Golden_GOW2` and `Golden_GOWR` parse real truncated game WADs
(`tests/fixtures/gow2/wad_minimal.wad` from `R_BOAR00.WAD`, PS2 USA;
`tests/fixtures/gowr/wad_minimal.wad` from `r_athena00.wad`, PC) against
pinned JSON snapshot goldens — they are the regression gate for parser
changes. Fixture provenance is documented in `tests/fixtures/README.md`;
`tools/make_test_fixtures.py` regenerates them.

**Baseline is green: 19/19, 67 doctest cases, 584 assertions.**

Earlier revisions of this file recorded a 5/7 baseline and blamed a theme
regression -- "the Onyx theme engine now returns 0.38-0.55" for dark
surfaces. That diagnosis was wrong, and the correction is worth keeping
because the same trap is easy to fall into again. `ThemeContrast` called
`Theme::ApplyTheme()` and then read `ImGui::GetStyle()`, but under v1.1
`ApplyTheme` only mutates the Appearance module's desired state; the style
is written later by `Appearance::Commit()`, which `Window` drives. With no
Window there is no Commit, so the test was measuring ImGui's own
`StyleColorsDark` defaults and calling them the theme -- which is why every
accent produced identical numbers and Dark was indistinguishable from
Light. The failing value was always exactly ImGui's default blue.

`Appearance::Resolve(state, env)` is the derivation itself, documented as
"Pure -- safe to call from tests with no context". The test uses it now, and
asserts the WCAG 2.1 contrast ratio between the theme's own text colour and
each surface rather than the old luminance proxy. Measured headroom: 5.48:1
worst case in Dark, 9.23:1 in Light, against a 4.5:1 (WCAG AA) floor.

## Architecture Overview

The application is built on **OnyxSDK**, an external engine library consumed
via CMake `FetchContent` (see `CMakeLists.txt`). Onyx owns the window/App
lifecycle, ImGui docking shell, asset database, profile dispatch, schema
system, VFS, and rendering. This repo (`Source/`) supplies the GoW-specific
content on top: format parsers, per-game profile logic, byte-layout schemas,
and the surviving game-specific UI panels/viewers. Entry point is
`Source/main.cpp`, which selects GUI (default, via `Onyx::App::Window`) or
CLI mode (`Source/cli/CliApp`) at startup.

**Rendering is Vulkan.** The pin is OnyxSDK `v1.1.0` (`CMakeLists.txt:17`),
which is Vulkan 1.3 via volk + VMA. There is no OpenGL anywhere: `glad` is
gone from the dependency set, `HeadlessGL`/`RenderCommand` were deleted
rather than ported (v1.1 has `Rendering::RenderToImage` and
`Cli::CmdRender`), and `MaterialViewer`'s private GL texture cache moved to
`App::TexturePool`. A viewer that wants an `ImTextureID` goes through
`TexturePool`; nothing in this repo touches a GL call.

`Onyx::Onyx` aggregates Core + Render + Shell only. The CLI additionally
links `Onyx::CliRender` (for `CmdRender`) and `Onyx::Exchange` (glTF export)
-- see `cmake/GoWToolkit.cmake`. Note that `Onyx::Cli::MakeGltfExportFn` is
declared in a public header but defined only inside Onyx's own example
executable, so it cannot be linked; `Source/cli/CliApp.cpp` builds the same
hook locally over `Exchange::ExportSceneData`.

### What Onyx owns (do not look for these in this repo)

- **Window/App lifecycle** — `<Onyx/App/Window.h>`, `<Onyx/App/App.h>`. This
  repo's `Source/AppRegistration.{h,cpp}` is a thin registrar shim that
  installs the GoW-specific panels/viewers onto the engine-generic `App`
  before it initializes; it is not the app coordinator itself.
- **Workspace / documents / modules** -- `<Onyx/Modules/Workspace.h>`. v1.1
  retired the profile layer entirely: `AssetDatabase`, `ProfileManager` and
  `IAssetProfile` no longer exist. A consumer implements `IGameModule` and
  registers it on a `Workspace`, which owns `Document`s. Selection is a
  `SelectionChanged{DocumentId, NodePath}` event on the Workspace's bus --
  `Onyx::Api::GetSelected()`/`Database()` are gone, and a holder re-resolves
  its path each frame instead of caching an `AssetEntry*`.
- **Schema tree** — `<Onyx/Schema/AssetFormat.h>` and friends replace the old
  local `StructDef`/`NodeInstance` system.
- **VFS** — `<Onyx/Vfs/...>` (`IFile`, `SliceFile`, ISO/PAK abstractions).
- **Rendering** -- scene rendering is entirely Onyx-owned; there is no local
  `rendering/` directory. `SceneRendererVk` is private to `Viewport3D`, which
  exposes no batch list and no visibility API -- which is why the GOWR LOD
  picker (`GowrLodDocument::Apply`) is currently inert and says so in a log
  line rather than silently ignoring the click. Restoring it needs a
  visibility entry point on `Viewport3D`, i.e. an OnyxSDK change.
- `IAssetLoader` as a distinct interface does not exist anywhere in the
  current codebase.

### What is local (`Source/`)

- `Source/core/parsers/gow2/` and `Source/core/parsers/gowr/` — the format
  parsers (mesh, texture, material, animation, etc.), touching only leaf Onyx
  API (`Vfs::IFile`, `Logger`, `Parsers::*Data`).
- `Source/core/modules/` — `Gow2Module` and `GowrModule`, the two
  `IGameModule` implementations a `Workspace` dispatches into. They replaced
  `ProfileGOW2`/`ProfileGOWR`. Probing scores evidence 0–100 and the winner
  must clear 40 *and* beat the runner-up, so both modules score positive
  evidence rather than deferring by exclusion (GOWR 95 on a real `WTOC`/LZ4
  header, GOW2 90 on `.iso` / 80 on a readable tag stream / 45 on a bare
  `.wad`).
- `Source/core/profiles/gow2/` and `Source/core/profiles/gowr/` — what
  survived the profile layer: the `WadNodeBuilder` tree construction and the
  GOWR naming taxonomy (`GowrTaxonomy`). `AssetEntry::profileTag` does not
  exist in v1.1, so `Gowr::Classify(entry)` reconstructs the role on demand
  from the entry's own name, size and typeId.

**A GOW2 tree carries two TypeId families at once**, and code that tests one
alone silently matches half a tree. `Gow2Module::ParseWadTagStream` stamps a
handler's legacy `Onyx::GameTypes::*` id (catalog keys like `GOW2_MODEL`)
whenever `WadTypeRegistry::ResolveByTag` finds one, and the module's own
`TypeRegistrar`-minted id (`gow2.model`) when it does not — so `list` on a
real WAD prints both spellings. `Gow2::SceneTypes` in
`core/types/Gow2SceneBuild.h` holds a pair per role and matches either; use
it rather than comparing against `GameTypes::*` directly. Note this differs
by target: the handler `.cpp` files are absent from `APP_TEST_SOURCES`, so
the test tree is pure module ids while the app gets the mix — which is why
the goldens cannot catch a mistake here.
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
- `Source/cli/` -- `CliApp`, which owns exactly one verb (`inspect`, which
  reports the scene an entry builds) and hands everything else to
  `Onyx::Cli::Run`: `probe`, `list`, `extract`, `decode`, `render`. All of
  them accept an ISO, because `Gow2Module` declares a `MountSpec` for it.
- `Source/AppRegistration.{h,cpp}` — see above.

## Platform Notes

**macOS**: Vulkan via MoltenVK. Uses `GLFW_DECORATED=TRUE` with NSWindow transparent titlebar + `NSVisualEffectView` glass effect (not GLFW borderless). Native menu bar built via `NSMenu` in Obj-C++. Smart drag via `macosSetWindowMovable`. Use `PathUtils::resolvePath()` for all resource paths (resolves relative to executable, critical inside `.app` bundle).

**Windows**: Borderless custom titlebar via Win32 HWND styling. FFmpeg DLLs copied post-build. A Release build is a GUI-subsystem binary, so the CLI attaches the parent console -- but only for a stream the caller did not already redirect, so `GoWToolkit list x.wad > out.txt` and pipes work.

**Linux**: Standard GLFW window. FFmpeg via pkg-config.

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
