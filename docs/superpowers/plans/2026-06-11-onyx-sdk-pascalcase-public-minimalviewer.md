# Onyx SDK — PascalCase + Public Surface + MinimalViewer (Plano 4: M3 + M4) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish turning the in-repo `Engine/` library into a presentable SDK: PascalCase every source directory, move every flat `Onyx::` type into a real sub-namespace taxonomy, lift the public headers into an `Engine/Include/Onyx/` surface that apps consume via `#include <Onyx/...>`, and prove the whole thing with a second consumer — `Apps/MinimalViewer` — that knows nothing about God of War and links only `Onyx`.

**Architecture:** Four independent, individually-green milestones, executed in this order to minimise rework: **M3a** renames `Engine/Source/{core,ui,window,rendering,fonts}` (and their subdirs) to PascalCase and rewrites every include path; **M3b** moves each subsystem from the flat `Onyx::` namespace into `Onyx::Vfs` / `Onyx::Schema` / `Onyx::Types` / `Onyx::Domain` / `Onyx::Parsers` / `Onyx::Audio` / `Onyx::Rendering` / `Onyx::App` / `Onyx::Viewers` / `Onyx::Services` (the already-sub-namespaced services — `Theme`, `Fonts`, `Scale`, `Threading`, `Api`, `Metrics`, `Platform` — are left untouched); **M3c** carves the public surface into `Engine/Include/Onyx/<Subsystem>/...`, makes `Include/` the only PUBLIC include root and `Source/` PRIVATE, and rewrites consumer includes to `<Onyx/...>`; **M4** adds `Apps/MinimalViewer`, a profile-less app that registers its own toy type and renders an app-authored hex viewer through the public API, with a `--selftest` headless path so it is ctest-verifiable.

**Tech Stack:** C++20, CMake + Ninja, MSVC. The golden GOW2/GOWR snapshot tests are the primary regression net for M3a–M3c (they exercise the engine through the app); the new `MinimalViewer_SelfTest` is the net for M4.

**Scope:** ONLY M3 (PascalCase + namespaces + public surface) and M4 (MinimalViewer). Deferred to Plano 5: physical repo extraction via `git filter-repo`, FetchContent consumption, `find_package`/install, and moving `MinimalViewer` into `Examples/`. NOT in scope: generalizing `WadBrowser`/`Inspector`/`MapViewer` (still App-side); renaming the `IGameProfile.h` *file* is done here (M3a) but the broader role/visuals-provider interface is future work; settling the final SDK name (`Onyx` stays placeholder).

---

## Environment (this machine)

`cl` is NOT on PATH. Enter the VS 2022 BuildTools dev shell **in the same shell call** as any build/test:

```powershell
Import-Module "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools" -DevCmdArguments "-arch=x64 -host_arch=x64" -SkipAutomaticLocation
```

A benign `'vswhere.exe' is not recognized` warning appears — ignore it. `cmake`/`ninja` are already on PATH. The canonical build dir is `build/` (configured against the post-Plano-3 top-level CMake).

**Canonical verify cycle (one shell call):**
```powershell
cmake --build build ; ctest --test-dir build --output-on-failure
```
Expected after every checkpoint: `100% tests passed` (7 tests in M3; 8 after M4 adds `MinimalViewer_SelfTest`).

**Commit hygiene:** `git add` explicit paths only — **never `git add -A`** (the `third_party/` submodules carry uncommitted deltas that must stay out). **No `Co-Authored-By` / "Generated with" / AI-credit trailers in any commit or PR** (standing user instruction).

**Case-insensitive filesystem gotcha (critical for M3a):** Windows (and macOS) treat `core` and `Core` as the same path. A direct `git mv core Core` is a no-op or errors. Every directory case-rename MUST go through a temp name:
```powershell
git mv Engine/Source/core Engine/Source/core_tmp
git mv Engine/Source/core_tmp Engine/Source/Core
```
Do the two-step for each directory, deepest-first is NOT required (renaming the parent moves children), but rename one directory level at a time and verify with `git status` after each.

---

## Pré-requisito: branch + baseline

- [ ] **Step 1: Confirm branch and green baseline**

```
git checkout feat/onyx-genericization
git status   # working tree clean except pre-existing third_party submodule deltas
```
Then (dev shell + build in one call): `cmake --build build ; ctest --test-dir build --output-on-failure` → expect `100% tests passed` (7). If not green, STOP and fix before starting.

---

# Milestone M3a — PascalCase directory rename

Rename `Engine/Source` directories (and their subdirectories) to PascalCase, preserving structure 1:1. CMake globs `Source/*.cpp` recursively so the build target list is unaffected; the only churn is **include-path strings** (`#include "core/..."` → `#include "Core/..."`) across **both** `Engine/Source` and the consumers (`Apps/GoWToolkit/Source`, `tests`), because `Engine/Source` is the shared include root.

**Directory rename map (every path segment → PascalCase, structure unchanged):**

| Current | New |
|---|---|
| `Engine/Source/core` | `Engine/Source/Core` |
| `Engine/Source/core/audio` | `Engine/Source/Core/Audio` |
| `Engine/Source/core/domain` | `Engine/Source/Core/Domain` |
| `Engine/Source/core/interfaces` | `Engine/Source/Core/Interfaces` |
| `Engine/Source/core/parsers` | `Engine/Source/Core/Parsers` |
| `Engine/Source/core/parsers/shared` | `Engine/Source/Core/Parsers/Shared` |
| `Engine/Source/core/platform` | `Engine/Source/Core/Platform` |
| `Engine/Source/core/schema` | `Engine/Source/Core/Schema` |
| `Engine/Source/core/types` | `Engine/Source/Core/Types` |
| `Engine/Source/core/vfs` | `Engine/Source/Core/Vfs` |
| `Engine/Source/fonts` | `Engine/Source/Fonts` |
| `Engine/Source/rendering` | `Engine/Source/Rendering` |
| `Engine/Source/ui` | `Engine/Source/Ui` |
| `Engine/Source/ui/platform` | `Engine/Source/Ui/Platform` |
| `Engine/Source/ui/viewers` | `Engine/Source/Ui/Viewers` |
| `Engine/Source/window` | `Engine/Source/Window` |
| `Engine/Source/window/platform` | `Engine/Source/Window/Platform` |

**Include-path rewrite map (applied to every `#include "..."` in `Engine/Source`, `Apps/GoWToolkit/Source`, `tests`):** the leading segment of an engine include is lowercased today; rewrite the prefixes `core/`→`Core/`, `ui/`→`Ui/`, `window/`→`Window/`, `rendering/`→`Rendering/`, `fonts/`→`Fonts/`. Because subdirs are also PascalCased, the full segment list must be rewritten: `core/types/` → `Core/Types/`, `core/vfs/` → `Core/Vfs/`, `ui/viewers/` → `Ui/Viewers/`, `core/parsers/shared/` → `Core/Parsers/Shared/`, etc.

> **App-side note:** `Apps/GoWToolkit/Source` keeps its OWN dirs lowercase for now (this milestone PascalCases the *engine* only — the app is a consumer, not the SDK). The app's includes of *engine* headers (e.g. `#include "core/types/TypeId.h"`) get rewritten to the new PascalCase engine paths; the app's includes of its *own* headers (e.g. `#include "core/profiles/gow2/ProfileGOW2.h"`) stay lowercase. The rewrite must therefore be path-aware — see Step 3.

- [ ] **Step 1: Rename engine directories (two-step per dir, case-safe)**

In the repo root, run (PowerShell), renaming parents first so children come along, then fixing nested-dir case under each parent. Do it level by level and `git status` between levels:

```powershell
# Level 1: top-level engine dirs
foreach ($p in @(
    @('Engine/Source/core','Engine/Source/Core'),
    @('Engine/Source/ui','Engine/Source/Ui'),
    @('Engine/Source/window','Engine/Source/Window'),
    @('Engine/Source/rendering','Engine/Source/Rendering'),
    @('Engine/Source/fonts','Engine/Source/Fonts'))) {
  git mv $p[0] ($p[0] + '_tmp'); git mv ($p[0] + '_tmp') $p[1]
}
```
Then the second level (now under the PascalCased parents):
```powershell
foreach ($p in @(
    @('Engine/Source/Core/audio','Engine/Source/Core/Audio'),
    @('Engine/Source/Core/domain','Engine/Source/Core/Domain'),
    @('Engine/Source/Core/interfaces','Engine/Source/Core/Interfaces'),
    @('Engine/Source/Core/parsers','Engine/Source/Core/Parsers'),
    @('Engine/Source/Core/platform','Engine/Source/Core/Platform'),
    @('Engine/Source/Core/schema','Engine/Source/Core/Schema'),
    @('Engine/Source/Core/types','Engine/Source/Core/Types'),
    @('Engine/Source/Core/vfs','Engine/Source/Core/Vfs'),
    @('Engine/Source/Ui/platform','Engine/Source/Ui/Platform'),
    @('Engine/Source/Ui/viewers','Engine/Source/Ui/Viewers'),
    @('Engine/Source/Window/platform','Engine/Source/Window/Platform'))) {
  git mv $p[0] ($p[0] + '_tmp'); git mv ($p[0] + '_tmp') $p[1]
}
```
Then the third level:
```powershell
git mv Engine/Source/Core/Parsers/shared Engine/Source/Core/Parsers/shared_tmp
git mv Engine/Source/Core/Parsers/shared_tmp Engine/Source/Core/Parsers/Shared
```
Verify: `git status --short | Select-String '_tmp'` returns nothing; `Get-ChildItem Engine/Source -Directory -Recurse | Select-Object FullName` shows all PascalCase.

- [ ] **Step 2: Verify the rename is staged as renames (no content change yet)**

```
git status --short
```
Expected: a block of `R ` (rename) entries `Engine/Source/core/... -> Engine/Source/Core/...`. No `M` on these files yet (content untouched). Do NOT build yet — includes are now broken (they still say `core/...`).

- [ ] **Step 3: Rewrite include paths (path-aware, engine paths only)**

The engine include prefixes to rewrite are exactly these five leading segments: `core/`, `ui/`, `window/`, `rendering/`, `fonts/`. Rewrite them to PascalCase **in `#include "..."` directives** across `Engine/Source`, `Apps/GoWToolkit/Source`, and `tests`. Crucially, the app's lowercase prefixes that point at engine subpaths must change, but the app ALSO has its own `core/...` headers — those resolve from the app's own `Source` root and the PascalCased engine `Core/` would shadow them. To stay unambiguous, rewrite by **full engine-relative path** rather than bare prefix: only rewrite an include if the target exists under `Engine/Source` after PascalCasing.

Use this PowerShell pass (runs in repo root; rewrites only includes whose PascalCased target is a real engine header):

```powershell
# Build the set of engine-relative header paths (PascalCase) that exist.
$engineRoot = "Engine/Source"
$engineHeaders = Get-ChildItem $engineRoot -Recurse -Include *.h,*.hpp |
    ForEach-Object { $_.FullName.Substring((Resolve-Path $engineRoot).Path.Length + 1).Replace('\','/') }
$engineSet = [System.Collections.Generic.HashSet[string]]::new()
foreach ($h in $engineHeaders) { [void]$engineSet.Add($h) }

# Lowercase->actual lookup: map a lowercased engine path to its real PascalCase path.
$lcToReal = @{}
foreach ($h in $engineHeaders) { $lcToReal[$h.ToLower()] = $h }

$targets = Get-ChildItem Engine/Source, Apps/GoWToolkit/Source, tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m
foreach ($f in $targets) {
    $changed = $false
    $lines = Get-Content -LiteralPath $f.FullName
    for ($i=0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\s*#\s*include\s*"([^"]+)"') {
            $inc = $Matches[1]
            $real = $lcToReal[$inc.ToLower()]
            if ($real -and $real -ne $inc) {
                $lines[$i] = $lines[$i].Replace('"' + $inc + '"', '"' + $real + '"')
                $changed = $true
            }
        }
    }
    if ($changed) { Set-Content -LiteralPath $f.FullName -Value $lines -Encoding UTF8 }
}
Write-Output "include rewrite done"
```

This only rewrites an include when the *lowercased* form matches a real PascalCase engine header — so an app-own `core/profiles/...` (which has no engine counterpart) is left alone, while `core/types/TypeId.h` → `Core/Types/TypeId.h` is fixed. (App-own headers under `core/` that happen to collide with an engine subpath do not exist — the M2 split put profiles/parsers/loaders/handlers under app-only subpaths like `core/profiles`, `core/loaders`, `core/types/handlers`, `core/types/GameTypes.h`, none of which exist under `Engine/Source/Core` — verify with the boundary grep in Step 5 if a collision is suspected.)

- [ ] **Step 4: Build + test**

```
cmake --build build ; ctest --test-dir build --output-on-failure
```
Expected: green (7 passed). Common failures: a missed include (a header whose case the script didn't catch because the path was spelled differently, e.g. backslashes) → fix that include by hand to the PascalCase path and rebuild. CMake's `CONFIGURE_DEPENDS` glob picks up the renamed paths on the next build automatically; if CMake doesn't notice, touch `Engine/CMakeLists.txt` or reconfigure `cmake -B build`.

- [ ] **Step 5: Boundary sanity grep**

```powershell
# No engine source should still reference a lowercase engine include prefix:
Select-String -Path (Get-ChildItem Engine/Source -Recurse -Include *.h,*.hpp,*.cpp,*.mm).FullName `
    -Pattern '#\s*include\s*"(core|ui|window|rendering|fonts)/' | Select-Object -First 20
```
Expected: no matches. (App-own lowercase includes under `Apps/` are fine and expected.)

- [ ] **Step 6: Commit**

```
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): PascalCase Engine/Source directory layout"
```
Verify `git show --stat HEAD` shows the renames into `Engine/Source/Core`, `Engine/Source/Ui`, etc. and the include-string edits — no `third_party`.

**M3a checkpoint:** build + 7 golden/unit tests green; all engine dirs PascalCase; no engine source includes a lowercase prefix.

---

# Milestone M3b — Sub-namespace taxonomy

Move each engine subsystem out of the flat `Onyx::` namespace into a dedicated sub-namespace. Do it **one subsystem per commit** so each is bisectable and the compiler guides reference fixups. The services already in sub-namespaces (`Onyx::Theme`, `Onyx::Fonts`, `Onyx::Scale`, `Onyx::Threading`, `Onyx::Api`, `Onyx::Metrics`, `Onyx::Platform`) and the external `PathUtils::` namespace are **left untouched**.

**Authoritative namespace map (post-M3a paths):**

| Dir | Target namespace | Key types moving out of flat `Onyx::` |
|---|---|---|
| `Core/Vfs` | `Onyx::Vfs` | `IVirtualFileSystem`, `IFile`, `IsoFileSystem`, `IsoEntry`, `IsoFile`, `OsFile`, `MemoryFile`, `SliceFile` |
| `Core/Schema` | `Onyx::Schema` | `StructDef`, `NodeInstance`, `AssetNode` (+ `StructNode`/`ArrayNode`/… node types, `NodeKind`), `AssetReader`, `AssetFormat`, `DataType`, `FieldDef`, `StructDefinition`, `DisplayHint`, `FieldType`, `SchemaField` |
| `Core/Types` | `Onyx::Types` | `TypeId`, `GameVersion`, `ITypeHandler`, `Color4f`, `TypeRegistry`, `TypeCatalog`, `TypeInfo` |
| `Core/Domain` | `Onyx::Domain` | `MediaKind` (+ `Name`/`Icon`), `AssetEntry`, `WadAssetName`, `AssetContainer`, `BoundingBox`, `GpuVertex`, `ProfileTag` |
| `Core/Interfaces` | `Onyx::Domain` | `IAssetProfile` (folds in with the domain model it parses) |
| `Core/Parsers/Shared` | `Onyx::Parsers` | `SceneData`, `MaterialInfo`, `BlendMode`, `AnimationData` (+ its many helper structs), `MeshData`, `MeshPart`, `ObjectData`, `Joint`, `TextureData`, `ScriptTargetParser` |
| `Core/Audio` | `Onyx::Audio` | `AdpcmDecoder` |
| `Rendering` | `Onyx::Rendering` | `AnimationPlayer`, `AxisGizmo`, `Camera`, `CameraView`, `GpuMesh`, `GridRenderer`, `SceneRenderer`, `RenderBatch`, `ShaderManager` |
| `Window` | `Onyx::App` | `Window` |
| `Ui` (shell + panels) | `Onyx::App` | `IPanel`, `PanelRegistry`, `ViewerRegistry`, `App` (in `Engine/Source/App.{h,cpp}`), `IsoBrowser`, `PakBrowser`, `CameraPanel`, `StatusBar`, `SettingsWindow`, `TitleBar`, `WindowDecorator`, `NativeMenuBar`, `NativeWindow`, `FontDebuggerWindow`, `AssetNodeRenderer`, `InfoTab`, `Widgets`, `Formatting`, `TypeVisuals`, `ActiveAnimation`, `AnimationTimeline` |
| `Ui/Viewers` | `Onyx::Viewers` | `IDocumentContent`, `DocumentWindow`, `ImageViewer`, `TextEditorViewer`, `VideoPlayer`, `Viewport3D`, `AnimCurveView`, `Dopesheet`, `WadStatsView` |
| loose `Core/*.{h,cpp}` services | `Onyx::Services` | `AssetDatabase`, `ProfileManager`, `AppConfig`, `RecentFiles`, `EventManager` (keep its internal `impl::`), `TaskManager`, `Logger` (keep `Log::`), `AssetVisibility`, `Visibility`, `Events` (event structs) |

> **Why `Onyx::App` for both Window and Ui-shell:** the window shell, panel registry, viewer registry, and the `App` coordinator are one cohesive "application shell" layer; collapsing them avoids a circular `Onyx::Ui` ↔ `Onyx::Window` split. `Onyx::Viewers` stays separate because the document viewers are the most app-overridable surface.

**Execution pattern for EACH subsystem (repeat the loop):** change the namespace *declaration* in that subsystem's headers/sources, build, let the compiler enumerate every now-unqualified reference, qualify them (or add a `using` inside the referencing scope), build green, golden green, commit. **Do not** add a blanket `namespace Onyx { using namespace Vfs; }` shim — that defeats the taxonomy; qualify references explicitly.

The ordering below goes leaf-first (fewest external references first) so each step's fixups are smaller.

- [ ] **Step 1: `Core/Audio` → `Onyx::Audio`**

In `Engine/Source/Core/Audio/*.{h,cpp}`, change `namespace Onyx {` → `namespace Onyx::Audio {` (and the matching closing-brace comment). Build:
```
cmake --build build
```
Fix every error `'AdpcmDecoder': is not a member of 'Onyx'` by qualifying the reference `Onyx::AdpcmDecoder` → `Onyx::Audio::AdpcmDecoder` at each use site (grep `AdpcmDecoder` across `Engine/Source` and `Apps/GoWToolkit/Source` to find them). Rebuild until green, then:
```
ctest --test-dir build --output-on-failure
git add Engine/Source/Core/Audio Engine Apps/GoWToolkit/Source
git commit -m "refactor(engine): Move Core/Audio into Onyx::Audio"
```

- [ ] **Step 2: `Core/Vfs` → `Onyx::Vfs`**

Change `namespace Onyx {` → `namespace Onyx::Vfs {` in every `Engine/Source/Core/Vfs/*.{h,cpp}`. Build; qualify all flagged references (`Onyx::IFile`→`Onyx::Vfs::IFile`, `Onyx::OsFile`, `Onyx::IsoFileSystem`, `Onyx::IsoEntry`, `Onyx::MemoryFile`, `Onyx::SliceFile`, `Onyx::IVirtualFileSystem`) across `Engine/Source`, `Apps/GoWToolkit/Source`, `tests`. Note `AssetDatabase` holds `std::vector<std::shared_ptr<Onyx::IsoFileSystem>>` — update that member type too. Build green, golden green, commit:
```
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move Core/Vfs into Onyx::Vfs"
```

- [ ] **Step 3: `Core/Schema` → `Onyx::Schema`**

Same loop for `Engine/Source/Core/Schema/*`. Qualify references to `StructDef`, `NodeInstance`, `AssetNode` and its node subclasses, `NodeKind`, `AssetReader`, `AssetFormat`, `DataType`, `FieldDef`, `StructDefinition`, `FieldType`, `SchemaField`, `DisplayHint`. These are heavily used by the app's parsers and handlers, so expect many fixups in `Apps/GoWToolkit/Source/core/parsers` and `.../core/types/handlers`. Build green, golden green:
```
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move Core/Schema into Onyx::Schema"
```

- [ ] **Step 4: `Core/Domain` + `Core/Interfaces` → `Onyx::Domain`**

Change both dirs' files to `namespace Onyx::Domain {`. `Entry.h`/`Wad.h` are partly global today — wrap their declarations in `namespace Onyx::Domain {}`. Qualify the high-traffic types `MediaKind`, `AssetEntry`, `AssetContainer`, `WadAssetName`, `BoundingBox`, `GpuVertex`, `ProfileTag`, `IAssetProfile` everywhere (these touch the most files — engine viewers, app profiles, parsers, handlers, tests). `MediaKind` appears in `ViewerRegistry`, `TypeInfo`, and the handlers; update all. Build green, golden green:
```
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move Core/Domain and Core/Interfaces into Onyx::Domain"
```

> If the `MediaKind` churn is too large to land in one green step, split: first move `MediaKind` alone (it's in `Core/Domain/MediaKind.h`) into `Onyx::Domain`, commit; then the rest of Domain. Keep each step green.

- [ ] **Step 5: `Core/Parsers/Shared` → `Onyx::Parsers`**

Loop for `Engine/Source/Core/Parsers/Shared/*`. Qualify `SceneData`, `MaterialInfo`, `BlendMode`, `MeshData`, `MeshPart`, `ObjectData`, `Joint`, `TextureData`, `ScriptTargetParser`, and the `AnimationData` family. App parsers/viewers (`MaterialViewer`, `MapViewer`, mesh parsers) reference these — fix there. Build green, golden green:
```
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move Core/Parsers/Shared into Onyx::Parsers"
```

- [ ] **Step 6: `Core/Types` → `Onyx::Types`**

Loop for `Engine/Source/Core/Types/*`. This is the highest-traffic move: `TypeId`, `TypeCatalog`, `TypeInfo`, `TypeRegistry`, `GameVersion`, `ITypeHandler`, `Color4f` are referenced pervasively by app type-registration (`GameTypes.cpp`, `GameTypeTable.h`), handlers, profiles, and tests. Qualify all `Onyx::TypeId`→`Onyx::Types::TypeId`, etc. The persistence equivalence test (`typetable_equivalence_test`) and visibility test reference `TypeId`/`GameVersion` — update those too. Build green; run the full suite (the GTKC equivalence + golden tests are the guard here):
```
ctest --test-dir build --output-on-failure
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move Core/Types into Onyx::Types"
```

- [ ] **Step 7: `Rendering` → `Onyx::Rendering`**

Loop for `Engine/Source/Rendering/*`. Qualify `Camera`, `CameraView`, `SceneRenderer`, `RenderBatch`, `GpuMesh`, `GridRenderer`, `ShaderManager`, `AxisGizmo`, `AnimationPlayer`. Mostly referenced by `Viewport3D` and the camera panel (engine) plus app map/material viewers. Build green, golden green:
```
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move Rendering into Onyx::Rendering"
```

- [ ] **Step 8: `Ui/Viewers` → `Onyx::Viewers`**

Loop for `Engine/Source/Ui/Viewers/*`. Qualify `IDocumentContent`, `DocumentWindow`, `ImageViewer`, `TextEditorViewer`, `VideoPlayer`, `Viewport3D`, `AnimCurveView`, `Dopesheet`, `WadStatsView`. App viewers (`MaterialViewer`, `SoundPlayer`, `MapViewer`) derive from `IDocumentContent` — update their base-class qualification and overrides. `ViewerRegistry::Factory` returns `std::shared_ptr<IDocumentContent>` — update its signature. Build green, golden green:
```
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move Ui/Viewers into Onyx::Viewers"
```

- [ ] **Step 9: `Window` + `Ui` shell → `Onyx::App`**

Loop for `Engine/Source/Window/*`, `Engine/Source/Ui/*` (the non-`Viewers` files), and `Engine/Source/App.{h,cpp}`. Move `Window`, `App`, `IPanel`, `PanelRegistry`, `ViewerRegistry`, and the engine panels into `namespace Onyx::App {`. This touches `main.cpp`, `AppRegistration.{h,cpp}`, and every app panel (`WadBrowser`, `Inspector`) that derives from `IPanel` or calls `App::SetRegistrar`/`addPanel`. Update the registrar signature references (`Onyx::App::App&`, `Onyx::App::PanelRegistry&`). Build green, run full suite (the `viewer_registry_test` references `ViewerRegistry` — update it):
```
ctest --test-dir build --output-on-failure
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move app shell (Window, App, panels) into Onyx::App"
```

- [ ] **Step 10: loose `Core/*` services → `Onyx::Services`**

For the loose service files still in flat `Onyx::` or global scope — `AssetDatabase`, `ProfileManager`, `AppConfig`, `RecentFiles`, `EventManager`, `TaskManager`, `Logger`, `AssetVisibility`, `Events` — wrap their declarations in `namespace Onyx::Services {`. Keep `EventManager`'s internal `Onyx::impl::` as `Onyx::Services::impl::`, and `Logger`'s `Log::` as `Onyx::Services::Log::`. `AssetDatabase`, `AppConfig`, `RecentFiles` are currently GLOBAL — moving them into `Onyx::Services` changes every unqualified `AssetDatabase`/`AppConfig`/`RecentFiles` reference (in `App`, `Window`, `main.cpp`, `ToolkitApi`, tests). Note `Onyx::Api` (ToolkitApi) returns `AssetDatabase&`/`AppConfig&` — update those return types to `Onyx::Services::`. Build green, full suite green:
```
ctest --test-dir build --output-on-failure
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Move core services into Onyx::Services"
```

- [ ] **Step 11: Verify no flat-`Onyx::` engine types remain**

```powershell
# Engine headers should declare only sub-namespaces now (plus the kept ones).
Select-String -Path (Get-ChildItem Engine/Source -Recurse -Include *.h).FullName `
    -Pattern '^\s*namespace\s+Onyx\s*\{' | Select-Object Path,LineNumber
```
Expected: no matches (every engine namespace is now `Onyx::<Sub>`). The kept services (`Onyx::Theme` etc.) and `PathUtils::` are unaffected. If a stray `namespace Onyx {` remains, assign it per the map above.

**M3b checkpoint:** build + full suite green; every engine type lives in a sub-namespace; no `namespace Onyx {` (flat) declaration remains in `Engine/Source`.

---

# Milestone M3c — Public `Include/Onyx/` surface

Carve the public API headers (the 49 the app/tests include — see list below) out of `Engine/Source/**` into `Engine/Include/Onyx/<Subsystem>/...`, make `Engine/Include` the SOLE `PUBLIC` include root, demote `Engine/Source` to `PRIVATE`, add an umbrella `Onyx.h`, and rewrite all consumer includes to angle-bracket `<Onyx/...>` form. Internal-only headers stay under `Source/` (private).

**Public header → new location map.** Public headers move to `Engine/Include/Onyx/<Subsystem>/<File>.h`, where `<Subsystem>` follows the M3b namespace (so the include path mirrors the namespace: `Onyx::Vfs::IFile` ↔ `<Onyx/Vfs/IFile.h>`):

| Current (post-M3a) path | New public path |
|---|---|
| `Source/Core/Vfs/IFile.h` | `Include/Onyx/Vfs/IFile.h` |
| `Source/Core/Vfs/IsoFileSystem.h` | `Include/Onyx/Vfs/IsoFileSystem.h` |
| `Source/Core/Vfs/OsFile.h` | `Include/Onyx/Vfs/OsFile.h` |
| `Source/Core/Vfs/MemoryFile.h` | `Include/Onyx/Vfs/MemoryFile.h` |
| `Source/Core/Vfs/SliceFile.h` | `Include/Onyx/Vfs/SliceFile.h` |
| `Source/Core/Schema/StructDef.h` | `Include/Onyx/Schema/StructDef.h` |
| `Source/Core/Schema/AssetReader.h` | `Include/Onyx/Schema/AssetReader.h` |
| `Source/Core/Schema/AssetFormat.h` | `Include/Onyx/Schema/AssetFormat.h` |
| `Source/Core/Types/TypeId.h` | `Include/Onyx/Types/TypeId.h` |
| `Source/Core/Types/TypeRegistry.h` | `Include/Onyx/Types/TypeRegistry.h` |
| `Source/Core/Types/TypeCatalog.h` | `Include/Onyx/Types/TypeCatalog.h` |
| `Source/Core/Types/GameVersion.h` | `Include/Onyx/Types/GameVersion.h` |
| `Source/Core/Types/ITypeHandler.h` | `Include/Onyx/Types/ITypeHandler.h` |
| `Source/Core/Domain/MediaKind.h` | `Include/Onyx/Domain/MediaKind.h` |
| `Source/Core/Domain/Entry.h` | `Include/Onyx/Domain/Entry.h` |
| `Source/Core/Domain/Wad.h` | `Include/Onyx/Domain/Wad.h` |
| `Source/Core/Interfaces/IGameProfile.h` | `Include/Onyx/Domain/IAssetProfile.h` *(file renamed to match its type)* |
| `Source/Core/Parsers/Shared/SceneNode.h` | `Include/Onyx/Parsers/SceneNode.h` |
| `Source/Core/Parsers/Shared/AnimationData.h` | `Include/Onyx/Parsers/AnimationData.h` |
| `Source/Core/Parsers/Shared/MeshData.h` | `Include/Onyx/Parsers/MeshData.h` |
| `Source/Core/Parsers/Shared/ObjectData.h` | `Include/Onyx/Parsers/ObjectData.h` |
| `Source/Core/Parsers/Shared/TextureData.h` | `Include/Onyx/Parsers/TextureData.h` |
| `Source/Core/Parsers/Shared/ScriptTargetParser.h` | `Include/Onyx/Parsers/ScriptTargetParser.h` |
| `Source/Core/Audio/AdpcmDecoder.h` | `Include/Onyx/Audio/AdpcmDecoder.h` |
| `Source/Core/Platform/SystemTheme.h` | `Include/Onyx/Platform/SystemTheme.h` |
| `Source/Core/AssetDatabase.h` | `Include/Onyx/Services/AssetDatabase.h` |
| `Source/Core/ProfileManager.h` | `Include/Onyx/Services/ProfileManager.h` |
| `Source/Core/AppConfig.h` | `Include/Onyx/Services/AppConfig.h` |
| `Source/Core/AssetVisibility.h` | `Include/Onyx/Services/AssetVisibility.h` |
| `Source/Core/TaskManager.h` | `Include/Onyx/Services/TaskManager.h` |
| `Source/Core/Events.h` | `Include/Onyx/Services/Events.h` |
| `Source/Core/Logger.h` | `Include/Onyx/Services/Logger.h` |
| `Source/Core/Metrics.h` | `Include/Onyx/Services/Metrics.h` |
| `Source/Core/Threading.h` | `Include/Onyx/Services/Threading.h` |
| `Source/Core/ThemeManager.h` | `Include/Onyx/Services/ThemeManager.h` |
| `Source/Core/PathUtils.h` | `Include/Onyx/Services/PathUtils.h` |
| `Source/Core/ToolkitApi.h` | `Include/Onyx/Api/ToolkitApi.h` |
| `Source/App.h` | `Include/Onyx/App/App.h` |
| `Source/Window/Window.h` | `Include/Onyx/App/Window.h` |
| `Source/Ui/IPanel.h` | `Include/Onyx/App/IPanel.h` |
| `Source/Ui/Widgets.h` | `Include/Onyx/App/Widgets.h` |
| `Source/Ui/InfoTab.h` | `Include/Onyx/App/InfoTab.h` |
| `Source/Ui/ViewerRegistry.h` | `Include/Onyx/App/ViewerRegistry.h` |
| `Source/Ui/Viewers/IDocumentContent.h` | `Include/Onyx/Viewers/IDocumentContent.h` |
| `Source/Ui/Viewers/DocumentWindow.h` | `Include/Onyx/Viewers/DocumentWindow.h` |
| `Source/Ui/Viewers/ImageViewer.h` | `Include/Onyx/Viewers/ImageViewer.h` |
| `Source/Ui/Viewers/TextEditorViewer.h` | `Include/Onyx/Viewers/TextEditorViewer.h` |
| `Source/Ui/Viewers/VideoPlayer.h` | `Include/Onyx/Viewers/VideoPlayer.h` |
| `Source/Ui/Viewers/Viewport3D.h` | `Include/Onyx/Viewers/Viewport3D.h` |
| `Source/UIHelpers.h` | `Include/Onyx/App/UIHelpers.h` |
| `Source/Fonts/SFSymbols.h` | `Include/Onyx/Fonts/SFSymbols.h` |

> The `PakBrowser.h`, `IsoBrowser.h`, `StatusBar.h`, `SettingsWindow.h`, `CameraPanel.h` panel headers are NOT in the 49-header app-included set (the app doesn't include them directly — `App` constructs them internally). They stay PRIVATE under `Source/`. If M4 needs one publicly, promote it then (incremental, use-driven).

- [ ] **Step 1: Create the `Include/Onyx/` tree and move public headers (git mv)**

Create the subdir skeleton, then `git mv` each public header per the map. Example for one subsystem (repeat for all rows):
```powershell
New-Item -ItemType Directory -Force Engine/Include/Onyx/Vfs,Engine/Include/Onyx/Schema,Engine/Include/Onyx/Types,Engine/Include/Onyx/Domain,Engine/Include/Onyx/Parsers,Engine/Include/Onyx/Audio,Engine/Include/Onyx/Platform,Engine/Include/Onyx/Services,Engine/Include/Onyx/Api,Engine/Include/Onyx/App,Engine/Include/Onyx/Viewers,Engine/Include/Onyx/Fonts | Out-Null
git mv Engine/Source/Core/Vfs/IFile.h Engine/Include/Onyx/Vfs/IFile.h
git mv Engine/Source/Core/Vfs/IsoFileSystem.h Engine/Include/Onyx/Vfs/IsoFileSystem.h
# ... one git mv per row of the map above ...
git mv Engine/Source/Core/Interfaces/IGameProfile.h Engine/Include/Onyx/Domain/IAssetProfile.h
```
After moving, the now-empty `Source/Core/Interfaces` dir can be removed (`git status` will show it gone once empty).

- [ ] **Step 2: Add the umbrella header `Engine/Include/Onyx/Onyx.h`**

```cpp
#pragma once
// Onyx SDK — umbrella public header. Apps may include this for the full
// public surface, or include individual <Onyx/Subsystem/Header.h> directly.

// Core data
#include <Onyx/Vfs/IFile.h>
#include <Onyx/Vfs/IsoFileSystem.h>
#include <Onyx/Vfs/OsFile.h>
#include <Onyx/Vfs/MemoryFile.h>
#include <Onyx/Schema/StructDef.h>
#include <Onyx/Schema/AssetReader.h>
#include <Onyx/Types/TypeId.h>
#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Types/GameVersion.h>
#include <Onyx/Domain/MediaKind.h>
#include <Onyx/Domain/Entry.h>
#include <Onyx/Domain/Wad.h>
#include <Onyx/Domain/IAssetProfile.h>

// Services
#include <Onyx/Services/AssetDatabase.h>
#include <Onyx/Services/ProfileManager.h>
#include <Onyx/Services/AppConfig.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Services/Threading.h>
#include <Onyx/Api/ToolkitApi.h>

// App shell + viewers
#include <Onyx/App/Window.h>
#include <Onyx/App/App.h>
#include <Onyx/App/IPanel.h>
#include <Onyx/App/ViewerRegistry.h>
#include <Onyx/Viewers/IDocumentContent.h>
#include <Onyx/Viewers/DocumentWindow.h>
```

- [ ] **Step 3: Rewrite includes of the moved headers to `<Onyx/...>`**

Across `Engine/Source`, `Apps/GoWToolkit/Source`, and `tests`, rewrite every `#include "<old public path>"` to its `<Onyx/...>` form. Build the rename map from the table and apply (PowerShell, repo root):
```powershell
# Map: old quoted include path (post-M3a) -> new angle path.
$map = @{
  'Core/Vfs/IFile.h'             = 'Onyx/Vfs/IFile.h'
  'Core/Vfs/IsoFileSystem.h'     = 'Onyx/Vfs/IsoFileSystem.h'
  'Core/Vfs/OsFile.h'            = 'Onyx/Vfs/OsFile.h'
  'Core/Vfs/MemoryFile.h'        = 'Onyx/Vfs/MemoryFile.h'
  'Core/Vfs/SliceFile.h'         = 'Onyx/Vfs/SliceFile.h'
  'Core/Schema/StructDef.h'      = 'Onyx/Schema/StructDef.h'
  'Core/Schema/AssetReader.h'    = 'Onyx/Schema/AssetReader.h'
  'Core/Schema/AssetFormat.h'    = 'Onyx/Schema/AssetFormat.h'
  'Core/Types/TypeId.h'          = 'Onyx/Types/TypeId.h'
  'Core/Types/TypeRegistry.h'    = 'Onyx/Types/TypeRegistry.h'
  'Core/Types/TypeCatalog.h'     = 'Onyx/Types/TypeCatalog.h'
  'Core/Types/GameVersion.h'     = 'Onyx/Types/GameVersion.h'
  'Core/Types/ITypeHandler.h'    = 'Onyx/Types/ITypeHandler.h'
  'Core/Domain/MediaKind.h'      = 'Onyx/Domain/MediaKind.h'
  'Core/Domain/Entry.h'          = 'Onyx/Domain/Entry.h'
  'Core/Domain/Wad.h'            = 'Onyx/Domain/Wad.h'
  'Core/Interfaces/IGameProfile.h' = 'Onyx/Domain/IAssetProfile.h'
  'Core/Parsers/Shared/SceneNode.h'        = 'Onyx/Parsers/SceneNode.h'
  'Core/Parsers/Shared/AnimationData.h'    = 'Onyx/Parsers/AnimationData.h'
  'Core/Parsers/Shared/MeshData.h'         = 'Onyx/Parsers/MeshData.h'
  'Core/Parsers/Shared/ObjectData.h'       = 'Onyx/Parsers/ObjectData.h'
  'Core/Parsers/Shared/TextureData.h'      = 'Onyx/Parsers/TextureData.h'
  'Core/Parsers/Shared/ScriptTargetParser.h' = 'Onyx/Parsers/ScriptTargetParser.h'
  'Core/Audio/AdpcmDecoder.h'    = 'Onyx/Audio/AdpcmDecoder.h'
  'Core/Platform/SystemTheme.h'  = 'Onyx/Platform/SystemTheme.h'
  'Core/AssetDatabase.h'         = 'Onyx/Services/AssetDatabase.h'
  'Core/ProfileManager.h'        = 'Onyx/Services/ProfileManager.h'
  'Core/AppConfig.h'             = 'Onyx/Services/AppConfig.h'
  'Core/AssetVisibility.h'       = 'Onyx/Services/AssetVisibility.h'
  'Core/TaskManager.h'           = 'Onyx/Services/TaskManager.h'
  'Core/Events.h'                = 'Onyx/Services/Events.h'
  'Core/Logger.h'                = 'Onyx/Services/Logger.h'
  'Core/Metrics.h'               = 'Onyx/Services/Metrics.h'
  'Core/Threading.h'             = 'Onyx/Services/Threading.h'
  'Core/ThemeManager.h'          = 'Onyx/Services/ThemeManager.h'
  'Core/PathUtils.h'             = 'Onyx/Services/PathUtils.h'
  'Core/ToolkitApi.h'            = 'Onyx/Api/ToolkitApi.h'
  'App.h'                        = 'Onyx/App/App.h'
  'Window/Window.h'              = 'Onyx/App/Window.h'
  'Ui/IPanel.h'                  = 'Onyx/App/IPanel.h'
  'Ui/Widgets.h'                 = 'Onyx/App/Widgets.h'
  'Ui/InfoTab.h'                 = 'Onyx/App/InfoTab.h'
  'Ui/ViewerRegistry.h'          = 'Onyx/App/ViewerRegistry.h'
  'Ui/Viewers/IDocumentContent.h' = 'Onyx/Viewers/IDocumentContent.h'
  'Ui/Viewers/DocumentWindow.h'  = 'Onyx/Viewers/DocumentWindow.h'
  'Ui/Viewers/ImageViewer.h'     = 'Onyx/Viewers/ImageViewer.h'
  'Ui/Viewers/TextEditorViewer.h' = 'Onyx/Viewers/TextEditorViewer.h'
  'Ui/Viewers/VideoPlayer.h'     = 'Onyx/Viewers/VideoPlayer.h'
  'Ui/Viewers/Viewport3D.h'      = 'Onyx/Viewers/Viewport3D.h'
  'UIHelpers.h'                  = 'Onyx/App/UIHelpers.h'
  'Fonts/SFSymbols.h'            = 'Onyx/Fonts/SFSymbols.h'
}
$targets = Get-ChildItem Engine/Source, Engine/Include, Apps/GoWToolkit/Source, tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m
foreach ($f in $targets) {
  $lines = Get-Content -LiteralPath $f.FullName; $changed = $false
  for ($i=0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^\s*#\s*include\s*"([^"]+)"') {
      $inc = $Matches[1]
      if ($map.ContainsKey($inc)) {
        $lines[$i] = ($lines[$i] -replace [regex]::Escape('"'+$inc+'"'), ('<'+$map[$inc]+'>'))
        $changed = $true
      }
    }
  }
  if ($changed) { Set-Content -LiteralPath $f.FullName -Value $lines -Encoding UTF8 }
}
Write-Output "public include rewrite done"
```

- [ ] **Step 4: Update `Engine/CMakeLists.txt` — Include is PUBLIC, Source is PRIVATE**

Replace the `target_include_directories(Onyx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)` block with:
```cmake
target_include_directories(Onyx
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source
)
```
The engine's own `.cpp` files include private headers via the `Source` root (still PRIVATE-visible to the engine target) and public headers via `<Onyx/...>` (PUBLIC root). Consumers (`GoWToolkit`, tests, `MinimalViewer`) inherit only `Include/`, so they can ONLY reach `<Onyx/...>` — exactly the public surface. Remove the now-redundant `${CMAKE_CURRENT_SOURCE_DIR}/Source` from `Apps/GoWToolkit/CMakeLists.txt`'s `target_include_directories` if it duplicates the inherited PUBLIC root (the app keeps its own `Source` private root for its own headers).

- [ ] **Step 5: Reconfigure + build + test**

The header moves change the file set; reconfigure:
```
cmake -B build ; cmake --build build ; ctest --test-dir build --output-on-failure
```
Expected: green. Failures will be private headers the app still tried to include directly (e.g. the app reaches into an engine internal that wasn't promoted) — that's a real boundary signal: either promote that header to `Include/Onyx/` (add a row to the map, move it, rewrite) or refactor the app off it. Iterate until green.

- [ ] **Step 6: Verify the public boundary**

```powershell
# Consumers may include Onyx ONLY via <Onyx/...> — no app/test file should
# include an engine Source-relative path anymore:
Select-String -Path (Get-ChildItem Apps/GoWToolkit/Source, tests -Recurse -Include *.h,*.cpp,*.mm).FullName `
    -Pattern '#\s*include\s*"(Core|Ui|Window|Rendering|Fonts|App\.h|UIHelpers)' | Select-Object -First 30
```
Expected: no matches that resolve to engine headers (app-own `core/profiles/...` quoted includes are fine — they're app-private). Spot-check that `#include <Onyx/...>` appears in app files.

- [ ] **Step 7: Commit**

```
git add Engine Apps/GoWToolkit/Source tests
git commit -m "refactor(engine): Carve public Include/Onyx surface, demote Source to private"
```

**M3c checkpoint:** build + full suite green; `Engine/Include/Onyx/**` holds the public headers; `Engine/Source` is the engine's private include root; consumers reach the SDK only through `<Onyx/...>`.

---

# Milestone M4 — `Apps/MinimalViewer`: the second consumer

A tiny app that knows nothing about God of War: it registers its own toy type in `Onyx::Types::TypeCatalog`, opens a file from disk via `Onyx::Vfs::OsFile`, and renders the bytes in an **app-authored** hex viewer that implements the public `Onyx::Viewers::IDocumentContent` — proving the public viewer interface is consumable from outside the engine. It links **only `Onyx::Onyx`** (never `GoWToolkit`). A `--selftest` flag exercises the type-registration + file-read + hex-format path headlessly and exits 0, so it is ctest-verifiable without opening a window.

**Files:**
- Create: `Apps/MinimalViewer/CMakeLists.txt`
- Create: `Apps/MinimalViewer/Source/Main.cpp`
- Create: `Apps/MinimalViewer/Source/HexViewer.h`
- Create: `Apps/MinimalViewer/Source/HexViewer.cpp`
- Create: `Apps/MinimalViewer/Source/SelfTest.h`
- Create: `Apps/MinimalViewer/Source/SelfTest.cpp`
- Modify: root `CMakeLists.txt` (add `add_subdirectory(Apps/MinimalViewer)`)
- Modify: `tests/CMakeLists.txt` (register the `MinimalViewer_SelfTest` ctest)

- [ ] **Step 1: Write the self-test (the failing test first)**

Create `Apps/MinimalViewer/Source/SelfTest.h`:
```cpp
#pragma once
namespace MinimalViewer {
// Runs the headless self-test: registers a toy type, reads `path` via the
// public VFS, formats a hex dump, and validates the round-trip. Returns 0 on
// success, non-zero on failure. Used by --selftest and by ctest.
int RunSelfTest(const char* path);
}
```

Create `Apps/MinimalViewer/Source/SelfTest.cpp`:
```cpp
#include "SelfTest.h"
#include "HexViewer.h"

#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Domain/MediaKind.h>
#include <Onyx/Vfs/OsFile.h>

#include <cstdio>
#include <vector>
#include <cstdint>

namespace MinimalViewer {

int RunSelfTest(const char* path) {
    // 1) Register a toy type purely through the public catalog API.
    Onyx::Types::TypeInfo info;
    info.key   = "MINIMAL_RAW_BLOCK";
    info.label = "Raw Binary Block";
    info.media = Onyx::Domain::MediaKind::Raw;
    info.icon  = nullptr;
    Onyx::Types::TypeId id = Onyx::Types::TypeCatalog::Get().Register(info);
    if (!id.valid()) { std::fprintf(stderr, "selftest: type registration failed\n"); return 1; }
    if (Onyx::Types::TypeCatalog::Get().Media(id) != Onyx::Domain::MediaKind::Raw) {
        std::fprintf(stderr, "selftest: media routing wrong\n"); return 2;
    }

    // 2) Open the file via the public VFS and read all bytes.
    Onyx::Vfs::OsFile file(path);
    if (!file.IsValid()) { std::fprintf(stderr, "selftest: cannot open %s\n", path); return 3; }
    std::vector<uint8_t> bytes = file.ReadAll();
    if (bytes.empty()) { std::fprintf(stderr, "selftest: empty read\n"); return 4; }

    // 3) Format a hex dump and validate it is non-trivial.
    std::string dump = FormatHexDump(bytes, /*maxBytes=*/256);
    if (dump.find("0000") == std::string::npos) {
        std::fprintf(stderr, "selftest: hex dump missing offset column\n"); return 5;
    }
    std::printf("selftest OK: %zu bytes, type id=%u\n", bytes.size(), id.value);
    return 0;
}

}
```

> Confirm the exact `OsFile`/`TypeCatalog`/`TypeInfo`/`MediaKind`/`TypeId` signatures against the moved public headers while implementing (they were read during planning: `OsFile(const std::string&)`, `bool IsValid()`, `std::vector<uint8_t> ReadAll()`; `TypeCatalog::Get()`, `TypeId Register(const TypeInfo&, uint32_t=0)`, `MediaKind Media(TypeId)`; `TypeId{uint32_t value; bool valid()}`; `TypeInfo{std::string key,label; MediaKind media; const char* icon; float color[4]}`; `enum class MediaKind { ..., Raw }`). If a signature differs, match the header — do NOT guess.

- [ ] **Step 2: Write the hex viewer (app-authored `IDocumentContent`)**

Create `Apps/MinimalViewer/Source/HexViewer.h`:
```cpp
#pragma once
#include <Onyx/Viewers/IDocumentContent.h>
#include <string>
#include <vector>
#include <cstdint>

namespace MinimalViewer {

// Pure formatting helper (testable without ImGui): classic 16-byte-per-row
// hex dump with an offset column and an ASCII gutter, capped at maxBytes.
std::string FormatHexDump(const std::vector<uint8_t>& bytes, size_t maxBytes);

// An app-authored document viewer that renders raw bytes as hex. Proves the
// public Onyx::Viewers::IDocumentContent interface is implementable outside
// the engine.
class HexViewer : public Onyx::Viewers::IDocumentContent {
public:
    HexViewer(std::string name, std::vector<uint8_t> bytes);

    std::string GetName() const override { return m_name; }
    void Draw() override;

private:
    std::string m_name;
    std::vector<uint8_t> m_bytes;
    std::string m_dump;   // precomputed
};

}
```

Create `Apps/MinimalViewer/Source/HexViewer.cpp`:
```cpp
#include "HexViewer.h"
#include <imgui.h>
#include <cstdio>

namespace MinimalViewer {

std::string FormatHexDump(const std::vector<uint8_t>& bytes, size_t maxBytes) {
    const size_t n = bytes.size() < maxBytes ? bytes.size() : maxBytes;
    std::string out;
    char line[128];
    for (size_t row = 0; row < n; row += 16) {
        std::snprintf(line, sizeof(line), "%08zX  ", row);
        out += line;
        for (size_t col = 0; col < 16; ++col) {
            if (row + col < n) { std::snprintf(line, sizeof(line), "%02X ", bytes[row + col]); out += line; }
            else               { out += "   "; }
        }
        out += " ";
        for (size_t col = 0; col < 16 && row + col < n; ++col) {
            uint8_t b = bytes[row + col];
            out += (b >= 32 && b < 127) ? char(b) : '.';
        }
        out += "\n";
    }
    return out;
}

HexViewer::HexViewer(std::string name, std::vector<uint8_t> bytes)
    : m_name(std::move(name)), m_bytes(std::move(bytes)) {
    m_dump = FormatHexDump(m_bytes, m_bytes.size());
}

void HexViewer::Draw() {
    ImGui::TextUnformatted("Raw bytes (hex):");
    ImGui::Separator();
    ImGui::BeginChild("hexdump", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushFont(nullptr); // default font; monospace not required for the smoke app
    ImGui::TextUnformatted(m_dump.c_str());
    ImGui::PopFont();
    ImGui::EndChild();
}

}
```

> Verify `IDocumentContent`'s exact virtuals against `<Onyx/Viewers/IDocumentContent.h>` (planning read: `std::string GetName() const = 0; void Draw() = 0;` plus optional `DrawInspector()`, `GetEmbeddedViewport()`, `IsOpen()/SetOpen()`). Override only the pure virtuals; if the namespace is `Onyx::Viewers::IDocumentContent` per M3b, use that — adjust if M3b assigned a different namespace.

- [ ] **Step 3: Write `Main.cpp` (window path + `--selftest` path)**

Create `Apps/MinimalViewer/Source/Main.cpp`:
```cpp
#include "SelfTest.h"
#include "HexViewer.h"

#include <Onyx/App/Window.h>
#include <Onyx/App/App.h>
#include <Onyx/Services/Threading.h>
#include <Onyx/Vfs/OsFile.h>
#include <Onyx/Viewers/DocumentWindow.h>

#include <cstring>
#include <memory>
#include <vector>
#include <cstdint>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

static int RunGui(const char* optionalPath) {
    Onyx::Services::Threading::MarkMainThread();

    Onyx::App::Window::initNative();
    Onyx::App::Window window;

    // If a file was passed, queue a hex tab once the App is initialised. The
    // registrar runs inside App::init(), after the engine's generic panels.
    std::string path = optionalPath ? optionalPath : "";
    window.app().SetRegistrar([path](Onyx::App::App& app) {
        if (path.empty()) return;
        Onyx::Vfs::OsFile file(path);
        if (!file.IsValid()) return;
        std::vector<uint8_t> bytes = file.ReadAll();
        auto viewer = std::make_shared<MinimalViewer::HexViewer>(path, std::move(bytes));
        app.getDocumentWindow().AddTab(viewer);
    });

    window.run();
    return 0;
}

int main(int argc, char** argv) {
    // Headless self-test: `MinimalViewer --selftest <file>` → no window.
    if (argc >= 2 && std::strcmp(argv[1], "--selftest") == 0) {
        const char* path = (argc >= 3) ? argv[2] : argv[0]; // default: dump own exe
        return MinimalViewer::RunSelfTest(path);
    }
    const char* optionalPath = (argc >= 2) ? argv[1] : nullptr;
    return RunGui(optionalPath);
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif
```

> Confirm `Window::initNative()`, `Window::app()`, `App::SetRegistrar(std::function<void(App&)>)`, `App::getDocumentWindow()`, and `DocumentWindow::AddTab(std::shared_ptr<IDocumentContent>)` against the moved public headers (planning read those exact signatures). If `App::getDocumentWindow()` is not public, use the public `Onyx::Api::Documents()` facade (`<Onyx/Api/ToolkitApi.h>`) instead — `Onyx::Api::Documents().AddTab(viewer)` — which is the documented public path.

- [ ] **Step 4: Write `Apps/MinimalViewer/CMakeLists.txt`**

```cmake
# ── MinimalViewer — second Onyx consumer (boundary proof) ─────────────────
# Knows nothing about God of War. Links ONLY Onyx::Onyx. Reaches the SDK only
# through <Onyx/...> public headers (inherits Onyx's PUBLIC Include/ root).

file(GLOB MINIMALVIEWER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

add_executable(MinimalViewer ${MINIMALVIEWER_SOURCES})
target_include_directories(MinimalViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(MinimalViewer PRIVATE Onyx::Onyx)

if(WIN32)
    set_target_properties(MinimalViewer PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()

# Fonts: the engine's ImGui setup loads fonts from third_party/fonts relative
# to the executable (same convention as GoWToolkit). Copy them next to the exe.
if(NOT APPLE)
    add_custom_command(TARGET MinimalViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/third_party/fonts"
            "$<TARGET_FILE_DIR:MinimalViewer>/third_party/fonts"
        COMMENT "Copying fonts next to MinimalViewer")
endif()

# FFmpeg DLLs (Windows) — Onyx links ffmpeg_lib, so the runtime DLLs must sit
# beside any Onyx-consuming exe.
if(WIN32 AND FFMPEG_DLLS)
    add_custom_command(TARGET MinimalViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FFMPEG_DLLS} "$<TARGET_FILE_DIR:MinimalViewer>"
        COMMENT "Copying FFmpeg DLLs next to MinimalViewer")
endif()
```

- [ ] **Step 5: Wire MinimalViewer into the root build**

In the root `CMakeLists.txt`, next to `add_subdirectory(Apps/GoWToolkit)`, add:
```cmake
add_subdirectory(Apps/MinimalViewer)
```

- [ ] **Step 6: Register the headless self-test as a ctest**

In `tests/CMakeLists.txt`, after the existing test registrations, add a ctest that runs the built MinimalViewer with `--selftest` against a known-present file (the golden GOW2 fixture, or simply the MinimalViewer exe dumping itself — use a fixture file that exists in the repo). Prefer an existing committed test asset; if the golden tests reference one, reuse its path. Example using the MinimalViewer binary dumping its own bytes (always present, no fixture needed):
```cmake
add_test(NAME MinimalViewer_SelfTest
         COMMAND MinimalViewer --selftest $<TARGET_FILE:MinimalViewer>)
set_tests_properties(MinimalViewer_SelfTest PROPERTIES
         PASS_REGULAR_EXPRESSION "selftest OK")
```

- [ ] **Step 7: Configure + build + run the self-test**

```
cmake -B build ; cmake --build build ; ctest --test-dir build --output-on-failure
```
Expected: `Onyx`, `GoWToolkit`, `MinimalViewer`, and `gowtoolkit_tests` all build; `100% tests passed` (now 8 — the 7 prior plus `MinimalViewer_SelfTest`). If `MinimalViewer_SelfTest` fails, run it directly to see the stderr reason: `./build/Apps/MinimalViewer/MinimalViewer.exe --selftest ./build/Apps/MinimalViewer/MinimalViewer.exe`.

- [ ] **Step 8: Verify MinimalViewer is GoW-clean and links only Onyx**

```powershell
# (a) No GoW symbol or app include in MinimalViewer source:
Select-String -Path (Get-ChildItem Apps/MinimalViewer/Source -Recurse -Include *.h,*.cpp).FullName `
    -Pattern 'GameTypes|ProfileGOW|WadBrowser|MaterialViewer|GoWToolkit|Apps/GoWToolkit' | Select-Object -First 20
# (b) Only <Onyx/...> public includes (no engine Source-relative quoted paths):
Select-String -Path (Get-ChildItem Apps/MinimalViewer/Source -Recurse -Include *.h,*.cpp).FullName `
    -Pattern '#\s*include\s*"(Core|Ui|Window|Rendering)/' | Select-Object -First 20
```
Expected: (a) and (b) both empty. The CMake `target_link_libraries(MinimalViewer PRIVATE Onyx::Onyx)` already proves it does not link `GoWToolkit`.

- [ ] **Step 9: Commit**

```
git add Apps/MinimalViewer CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(minimalviewer): Add second Onyx consumer proving the public API"
```

**M4 checkpoint:** `MinimalViewer` builds and links only `Onyx`; `MinimalViewer_SelfTest` passes under ctest; the app authors its own `IDocumentContent` and registers its own type through `<Onyx/...>` alone — **the SDK is proven decoupled by a second, GoW-free consumer.**

---

## Verificação final do Plano 4

- [ ] **Step 1: Clean rebuild + full suite**

```
cmake --build build --target clean ; cmake --build build ; ctest --test-dir build --output-on-failure
```
Expected: 8/8 green from clean.

- [ ] **Step 2: Engine standalone + public-only consumption**

```
cmake --build build --target Onyx          # engine archives with zero app files
cmake --build build --target MinimalViewer # second consumer builds on <Onyx/...> alone
```
Both succeed. Combined with the M3c public-boundary grep and the M4 GoW-clean grep, this proves: (a) the engine is game-agnostic, (b) its public surface is self-sufficient, (c) a fresh app needs nothing but `Onyx::Onyx`.

- [ ] **Step 3: Confirm structure**

`Engine/Source/**` is PascalCase and PRIVATE; every engine type is in a sub-namespace; `Engine/Include/Onyx/**` is the public surface; `Apps/GoWToolkit` and `Apps/MinimalViewer` both consume `Onyx::Onyx` via `<Onyx/...>`. Plano 4 concluído.

---

## Self-review notes (spec coverage)

- **M3 — PascalCase (spec §4):** M3a. ✓
- **M3 — public Include/Onyx surface (spec §3, §4):** M3c (full split, per the user's "split completo agora" decision — note this is *more* than the spec's "incremental" suggestion; accepted deliberately). ✓
- **PascalCase + sub-namespaces (spec §0):** M3a (dirs) + M3b (full taxonomy, per the user's "taxonomia completa" decision). ✓
- **M4 — MinimalViewer, GoW-free second consumer (spec §4, §5):** M4, with a headless `--selftest` making it ctest-verifiable (stronger than the spec's "roda standalone" smoke). ✓
- **`IGameProfile.h` file rename (carry-forward):** done in M3c Step 1 (→ `Onyx/Domain/IAssetProfile.h`). ✓
- **Deferred (Plano 5):** repo split, FetchContent, `Examples/` move, `find_package`/install — explicitly out of scope. ✓

---

## Roadmap após o Plano 4
- **Plano 5 — Physical repo split (M5).** `git filter-repo` extract `Engine/` → `OnyxSDK` (preserve history); GoWToolkit consumes via `FetchContent` on a tag; move `MinimalViewer` → `OnyxSDK/Examples/`; tag `OnyxSDK v0.1.0`. **SCUMMRedux can then be born.**

### Carry-forward
- Generalize `WadBrowser`/`Inspector`/`MapViewer` (role/visuals provider interface) — still App-side.
- Settle the final SDK name before Plano 5 (`Onyx` still placeholder; check domain/crate/npm availability).
- The app's own `Apps/GoWToolkit/Source` dirs remain lowercase — PascalCase them too if consistency across the repo is wanted (cosmetic; not required for the SDK boundary).
