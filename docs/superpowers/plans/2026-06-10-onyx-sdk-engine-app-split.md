# Onyx SDK — Engine/App Split (Plano 3: M2) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split the single GoWToolkit target into an `Engine` static library (reusable, game-agnostic SDK) and an `Apps/GoWToolkit` executable (the first consumer), proving the boundary by making the engine compile and link with zero app files — while keeping the build and golden tests green.

**Architecture:** Four in-place decoupling refactors (each green) precede the structural move. The dominant blocker is one leak hub: `AssetEntry` (in `core/domain/Entry.h`) default-initializes its `typeId` to the app handle `GameTypes::Unknown`, dragging app code into every translation unit. Fixing that (+ removing one unused include) collapses the transitive leak set. Then PakBrowser is generalized via a profile predicate, `AssetVisibility` is split (generic store in Engine, GoW defaults registered by the app), and `App` keeps its generic DockSpace/menu/config in Engine while game panels/viewers are registered through an injected callback the executable supplies. Finally files physically move into `Engine/Source/<subpath>` and `Apps/GoWToolkit/Source/<subpath>` — **subpaths are preserved**, so existing `#include "core/…"` / `"ui/…"` lines keep resolving (the Engine target's `Source` dir is a PUBLIC include root the app inherits). PascalCase dirs + an `Include/Onyx/` public surface are deliberately NOT done here (that's the next plan).

**Tech Stack:** C++20, CMake + Ninja, MSVC. Golden GOW2/GOWR snapshot tests are the primary regression net.

**Scope:** ONLY M2 (the split). Deferred to later plans: PascalCase directory renames + `Include/Onyx/` public-header surface (M3), `MinimalViewer` (M4), physical repo extraction (M5). Browsers `WadBrowser`/`Inspector`/`MapViewer` and the format viewers stay App for now (they switch on GoW roles); their generalization is future work.

**Environment (this machine):** `cl` is not on PATH — enter the VS 2022 BuildTools dev shell IN THE SAME shell call as any build/test:
```powershell
Import-Module "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools" -DevCmdArguments "-arch=x64 -host_arch=x64" -SkipAutomaticLocation
```
For Tasks 1–4 the existing `build/` works (`cmake --build build`). **Task 5 changes the CMake structure** — it must reconfigure into a fresh build dir (`build2/`). When committing, `git add` explicit paths only (never `git add -A`; uncommitted `third_party/` deltas must stay out). **No `Co-Authored-By`/AI credit trailers in any commit** (standing user instruction).

**Boundary contract (the invariant the whole plan serves):** after Task 5, the `Engine` target compiles and links WITHOUT any file under `Apps/`. The Engine never includes an app header. Verification: a clean engine-only build + a grep that no `Engine/Source` file includes an `Apps/` path or names `GameTypes`/`Profile`/a game parser.

**Engine vs App placement (from the dependency map):**
- **Engine:** `window/**`, `rendering/**`, services (`Logger`,`Metrics`,`Threading`,`TaskManager`,`ThemeManager`,`FontManager`,`ScaleManager`,`AppConfig`,`RecentFiles`,`EventManager`,`Events.h`,`PathUtils.h`), `platform/SystemTheme*`, `fonts/SFSymbols.h`, `core/vfs/**`, `core/schema/**`, `core/audio/**`, `core/parsers/shared/**`, generic types (`TypeId`,`TypeCatalog`,`TypeRegistry`,`ITypeHandler`,`MediaKind`,`GameVersion`), generic domain (`Entry`,`Wad`,`MediaKind`,`BoundingBox`,`MeshVertex`,`ProfileTag`), `core/interfaces/IGameProfile.h`, `core/AssetDatabase`, `core/ProfileManager`, `core/ToolkitApi`, `core/AssetVisibility` (generic part — see Task 3), generic UI (`IPanel`,`PanelRegistry`,`ViewerRegistry`,`Widgets`,`Formatting`,`TitleBar`,`NativeMenuBar`,`WindowDecorator`,`NativeWindow*`,`CameraPanel`,`FontDebuggerWindow`,`StatusBar`,`SettingsWindow`,`IsoBrowser`,`PakBrowser`(after Task 2),`AssetNodeRenderer`,`InfoTab`,`AnimationTimeline`,`ActiveAnimation`,`TypeVisuals`), generic viewers (`IDocumentContent`,`DocumentWindow`,`ImageViewer`,`VideoPlayer`,`TextEditorViewer`,`Viewport3D`,`AnimCurveView`,`Dopesheet`,`WadStatsView`), `App.{h,cpp}` (after Task 4), `UIHelpers.h`.
- **App:** `core/profiles/**`, `core/parsers/gow2/**`, `core/parsers/gowr/**`, `core/loaders/**`, `core/types/handlers/**`, `core/types/GameTypes.*`, `core/types/GameTypeTable.h`, `core/formats/**`, `core/domain/WadEntryRoleLegacy.h`, `core/WadTypes.h`, viewers `MaterialViewer`,`SoundPlayer`,`MapViewer`, UI `WadBrowser*`,`Inspector`,`RoleVisuals.h`, `cli/**`, `main.cpp`, and the app-side AssetVisibility-defaults + panel-registrar (Tasks 3–4).

---

## Pré-requisito: branch + baseline

- [ ] **Step 1: Confirm branch and green baseline**

```
git checkout feat/onyx-genericization
```
Then (dev shell + build in one call): `cmake --build build ; ctest --test-dir build --output-on-failure` → expect `100% tests passed` (7). If not green, STOP.

---

## Task 1: Decouple the leak hub (Entry.h + ITypeHandler.h)

The two edits that collapse the transitive leak. Both are behavior-preserving (`TypeId{}` already equals value 0 == Unknown).

**Files:** Modify `src/core/domain/Entry.h`, `src/core/types/ITypeHandler.h`.

- [ ] **Step 1: Remove the app dependency from `Entry.h`**

In `src/core/domain/Entry.h`: delete the `#include "core/types/GameTypes.h"` line, and change the field initializer (around line 84) from
`Onyx::TypeId typeId = Onyx::GameTypes::Unknown;` to `Onyx::TypeId typeId = {};`
(`TypeId` default-constructs to `value = 0`, which is exactly `Unknown` — verify in `TypeId.h`). Keep the `#include "core/types/TypeId.h"`.

- [ ] **Step 2: Remove the unused app include from `ITypeHandler.h`**

In `src/core/types/ITypeHandler.h`: delete the `#include "GameTypes.h"` (or `core/types/GameTypes.h`) line. The header only references `GameVersion`, `TypeId`, `AssetNode`, `SceneData` — confirm by reading it that no `GameTypes::` symbol is used in the header body.

- [ ] **Step 3: Build + test**

`cmake --build build ; ctest --test-dir build --output-on-failure`
Expected: green. Some app `.cpp` files that relied on getting `GameTypes.h` transitively (via Entry.h/ITypeHandler.h) may now fail to compile — if so, add an explicit `#include "core/types/GameTypes.h"` to those specific files (they are app files, so this is correct). Iterate until green.

- [ ] **Step 4: Commit**

```
git add src/core/domain/Entry.h src/core/types/ITypeHandler.h src
git commit -m "refactor(core): Decouple AssetEntry and ITypeHandler from game types"
```
(Stage `src` to include any explicit-include fixups from Step 3; verify `git show --stat HEAD` has no third_party.)

---

## Task 2: Generalize PakBrowser via a profile container-predicate

PakBrowser's only game coupling (`PakBrowser.cpp:113-114`) is `entry.typeId == GameTypes::WadFile || entry.typeId == GameTypes::Unknown` to decide whether a tree node is an openable container. Replace with a predicate the profile provides.

**Files:** Modify `src/core/interfaces/IGameProfile.h` (add method), `src/core/profiles/gow2/ProfileGOW2.{h,cpp}`, `src/core/profiles/gowr/ProfileGOWR.{h,cpp}` (implement), `src/ui/PakBrowser.cpp` (use it).

- [ ] **Step 1: Add the predicate to the interface**

In `src/core/interfaces/IGameProfile.h`, add to `class IAssetProfile`:
```cpp
// True if this entry represents an openable container (e.g. a WAD/archive
// node the user can drill into). Default: false. Profiles override.
virtual bool IsContainerEntry(const AssetEntry& entry) const { return false; }
```
(Ensure `AssetEntry` is visible — the header already includes the domain types via `Wad.h`/`Entry.h`; add the include if needed.)

- [ ] **Step 2: Implement in both profiles**

In `ProfileGOW2` and `ProfileGOWR` (`.h` declare `bool IsContainerEntry(const AssetEntry&) const override;`, `.cpp` define):
```cpp
bool ProfileGOW2::IsContainerEntry(const AssetEntry& e) const {
    return e.typeId == GameTypes::WadFile || e.typeId == GameTypes::Unknown;
}
```
(Same body for `ProfileGOWR` unless its container semantics differ — check how PakBrowser used it; if GOWR has no PAK/WAD nesting, returning the same is safe since the golden tests will catch a behavior change. Add `#include "core/types/GameTypes.h"` to the profile `.cpp`.)

- [ ] **Step 3: Use it in PakBrowser**

In `src/ui/PakBrowser.cpp` (~line 113), replace the `GameTypes::` comparison with a call through the active profile. PakBrowser has access to the open container/profile (it browses `AssetContainer`); use `wad.profile->IsContainerEntry(entry)` (match the actual variable names in scope — read the function). Remove the now-unused `#include "core/types/GameTypes.h"` from `PakBrowser.cpp` if present. After this, `PakBrowser` must reference NO `GameTypes::` symbol (grep to confirm).

- [ ] **Step 4: Build + test + commit**

`cmake --build build ; ctest --test-dir build --output-on-failure` → green (golden confirms PAK browsing behavior unchanged).
```
git add src/core/interfaces/IGameProfile.h src/core/profiles src/ui/PakBrowser.cpp
git commit -m "refactor(ui): Generalize PakBrowser container test via profile predicate"
```

---

## Task 3: Split AssetVisibility (generic store in Engine, GoW defaults in App)

`AssetVisibility` is a generic (GameVersion, TypeId)→Visibility store + persistence, but `RegisterDefaults()` hardcodes GoW types. Keep the class generic; move the default-seeding into an app-provided function.

**Files:** Modify `src/core/AssetVisibility.{h,cpp}`; create `src/core/profiles/AssetVisibilityDefaults.{h,cpp}` (app); call it from startup (`src/main.cpp`).

- [ ] **Step 1: Make the store seedable from outside**

In `src/core/AssetVisibility.h`: replace the private `void RegisterDefaults();` with a public API the app uses to seed defaults:
```cpp
// Seed a default visibility for a (version, type). Called by the app at
// startup (the engine ships no game-specific defaults).
void SetDefault(GameVersion ver, TypeId id, Visibility vis);
```
Remove the `RegisterDefaults()` call from the constructor. In `AssetVisibility.cpp`: delete the `#include "core/types/GameTypes.h"` and the entire `RegisterDefaults()` body; implement `SetDefault` to write `m_defaults[MakeKey(ver,id)] = vis;`. The class now has NO `GameTypes::` reference (grep to confirm) → Engine-clean.

- [ ] **Step 2: Move the GoW defaults into an app file**

Create `src/core/profiles/AssetVisibilityDefaults.h`:
```cpp
#pragma once
namespace Onyx { void RegisterGameVisibilityDefaults(); }
```
Create `src/core/profiles/AssetVisibilityDefaults.cpp` — transcribe the OLD `RegisterDefaults()` body here, calling `AssetVisibility::Get().SetDefault(ver, GameTypes::X, vis)` for each line that used to be in the switch/table:
```cpp
#include "core/profiles/AssetVisibilityDefaults.h"
#include "core/AssetVisibility.h"
#include "core/types/GameTypes.h"
namespace Onyx {
void RegisterGameVisibilityDefaults() {
    auto& v = AssetVisibility::Get();
    // transcribe every old default, e.g.:
    v.SetDefault(GameVersion::GOW2, GameTypes::EntityCount, Visibility::Internal);
    // ... all remaining defaults from the old RegisterDefaults() ...
}
}
```
(Read the original `RegisterDefaults()` and transcribe ALL entries — do not abbreviate.)

- [ ] **Step 3: Call it at startup**

In `src/main.cpp`, after `RegisterGameTypes()`, add `Onyx::RegisterGameVisibilityDefaults();` (and the include). Do the same in `src/cli/CliApp.cpp` if the CLI relies on visibility (check; add only if needed).

- [ ] **Step 4: Add the new app sources to CMake & build**

Add `src/core/profiles/AssetVisibilityDefaults.cpp` to the build. (In the current single-target `CMakeLists.txt` it is picked up by the `GLOB_RECURSE src/*.cpp`; but it must NOT go into `PARSER_MIN_SOURCES`/the engine set later. For now the GLOB covers it.) Build + test:
`cmake --build build ; ctest --test-dir build --output-on-failure` → green. The golden tests confirm visibility defaults still apply (the tree filtering is unchanged).

> If a golden test depends on visibility defaults being present during the test harness run, ensure the test harness also calls `RegisterGameVisibilityDefaults()` — check `tests/golden_helpers.cpp`; add the call alongside its existing `RegisterGameTypes()` if the golden output reflects visibility.

- [ ] **Step 5: Commit**

```
git add src/core/AssetVisibility.h src/core/AssetVisibility.cpp src/core/profiles/AssetVisibilityDefaults.h src/core/profiles/AssetVisibilityDefaults.cpp src/main.cpp src/cli/CliApp.cpp tests
git commit -m "refactor(core): Split AssetVisibility store from game-specific defaults"
```

---

## Task 4: Inject app panel/viewer registration into App

`App::registerPanels()` directly instantiates game panels/viewers (`WadBrowser`, `MaterialViewer`, `SoundPlayer`, `MapViewer` via ViewerRegistry, `Inspector`). For `App` to live in the Engine, that game wiring must come from the executable.

**Files:** Modify `src/App.{h,cpp}`; create `src/AppRegistration.{h,cpp}` (app-side registrar); modify `src/main.cpp` (supply the registrar).

- [ ] **Step 1: Add an injected registrar to App**

In `src/App.h`, add (engine side):
```cpp
#include <functional>
// ...
public:
    // Hook the executable uses to register game-specific panels and viewers.
    // Called once during init, after the engine's generic panels are set up.
    using AppRegistrar = std::function<void(PanelRegistry&, Onyx::ViewerRegistry&, /*AppContext&*/ ...)>;
    void SetRegistrar(AppRegistrar r) { m_registrar = std::move(r); }
private:
    AppRegistrar m_registrar;
```
(Match the ACTUAL signature your `registerPanels()` needs — read it to see what context the game panels require: `PanelRegistry`, the `ViewerRegistry`, and the `AppContext`. Pass exactly those.)

- [ ] **Step 2: Split `registerPanels()` into engine-generic + injected app part**

In `src/App.cpp`, `registerPanels()`: keep registration of the GENERIC panels that are Engine (e.g. `IsoBrowser`, `PakBrowser`, `StatusBar`, `SettingsWindow`, `CameraPanel`, `FontDebuggerWindow` — whatever is Engine per the placement list). REMOVE the instantiation of the game panels/viewers (`WadBrowser`, `Inspector`, and the `ViewerRegistry` factories for `MaterialViewer`/`SoundPlayer`/`MapViewer`). After registering the generic panels, call `if (m_registrar) m_registrar(m_panels, m_viewerRegistry, /*context*/);`.
After this, `App.cpp` must include NO game header (no `WadBrowser.h`, `Inspector.h`, `MaterialViewer.h`, `SoundPlayer.h`, `MapViewer.h`) and reference no `GameTypes::`/profile symbol — grep to confirm. App is now Engine-clean.

- [ ] **Step 3: Create the app-side registrar**

Create `src/AppRegistration.h`:
```cpp
#pragma once
class App;
namespace Onyx { void InstallGoWPanels(App& app); }
```
Create `src/AppRegistration.cpp` — this is APP code; it includes the game panels/viewers and installs them via `app.SetRegistrar(...)`:
```cpp
#include "AppRegistration.h"
#include "App.h"
#include "ui/WadBrowser.h"
#include "ui/Inspector.h"
#include "ui/viewers/MaterialViewer.h"
#include "ui/viewers/SoundPlayer.h"
#include "ui/viewers/MapViewer.h"
// ...
namespace Onyx {
void InstallGoWPanels(App& app) {
    app.SetRegistrar([](PanelRegistry& panels, Onyx::ViewerRegistry& viewers /*, AppContext& ctx*/) {
        // Move the removed registrations here verbatim:
        panels.add(std::make_unique<WadBrowser>(/*...*/));
        panels.add(std::make_unique<Inspector>(/*...*/));
        viewers.Register(MediaKind::Material, /* MaterialViewer factory */);
        viewers.Register(MediaKind::Audio,    /* SoundPlayer factory */);
        viewers.Register(MediaKind::Map,      /* MapViewer factory */);
        // ... exactly what App::registerPanels() used to do for game UI ...
    });
}
}
```
(Transcribe the EXACT registrations you removed from `App.cpp` Step 2, with their real constructor args/context. Read the original `registerPanels()` to get them right.)

- [ ] **Step 4: Wire it in main**

In `src/main.cpp`, after constructing the `App`/`Window` (read how `Window` owns `App m_app` — you may need a `Window::app()` accessor, or set the registrar on the `App` before the loop), call `Onyx::InstallGoWPanels(<the App instance>);` before `App::init`/the loop runs. Add `#include "AppRegistration.h"`. If `Window` owns `App` privately, add a minimal accessor `App& Window::app() { return m_app; }` (engine-side, generic).

- [ ] **Step 5: Build + test + commit**

`cmake --build build ; ctest --test-dir build --output-on-failure` → green. Manually confirm (or trust golden + a smoke run) that the game panels still appear. 
```
git add src/App.h src/App.cpp src/AppRegistration.h src/AppRegistration.cpp src/main.cpp src/window
git commit -m "refactor(app): Inject game panel/viewer registration into App"
```

---

## Task 5: The structural split — Engine library + Apps/GoWToolkit executable

Physically relocate files (preserving subpaths) and rewrite CMake into two targets. After Tasks 1–4, no Engine file depends on an app file, so this is mechanical.

**New layout (subpaths preserved so includes keep resolving):**
```
Engine/
  CMakeLists.txt
  Source/                      ← PUBLIC include root (so #include "core/..." resolves)
    window/  rendering/  fonts/
    core/{vfs,schema,audio,types(generic),domain(generic),parsers/shared,
          interfaces,platform, + service .cpp/.h, AssetDatabase, ProfileManager,
          ToolkitApi, AssetVisibility}
    ui/{generic panels/viewers/widgets}  App.{h,cpp}  UIHelpers.h
Apps/
  GoWToolkit/
    CMakeLists.txt
    Source/                    ← include root for app
      main.cpp  AppRegistration.{h,cpp}
      core/{profiles, parsers/gow2, parsers/gowr, loaders, types/handlers,
            types/GameTypes.*, types/GameTypeTable.h, formats, domain/WadEntryRoleLegacy.h, WadTypes.h}
      ui/{WadBrowser*, Inspector, RoleVisuals.h, viewers/MaterialViewer, viewers/SoundPlayer, viewers/MapViewer}
      cli/
```

- [ ] **Step 1: Create dirs and move ENGINE files (git mv, preserve subpaths)**

Create `Engine/Source/`. `git mv` the engine files there preserving their `core/…`,`ui/…`,`window/…`,`rendering/…`,`fonts/…` subpaths. Move whole clean subtrees first (`git mv src/window Engine/Source/window`, `git mv src/rendering Engine/Source/rendering`, `git mv src/core/vfs Engine/Source/core/vfs`, `git mv src/core/schema Engine/Source/core/schema`, `git mv src/core/audio Engine/Source/core/audio`, `git mv src/core/parsers/shared Engine/Source/core/parsers/shared`, `git mv src/core/platform Engine/Source/core/platform`, `git mv src/fonts Engine/Source/fonts`), then move individual engine files from mixed dirs (`core/types/*` generic ones, `core/domain/*` generic ones, the service files, `core/interfaces`, `AssetDatabase`,`ProfileManager`,`ToolkitApi`,`AssetVisibility`, the generic `ui/*` and `ui/viewers/*`, `App.*`, `UIHelpers.h`). Use the Engine placement list above as the authoritative set.

- [ ] **Step 2: Move APP files**

Create `Apps/GoWToolkit/Source/`. `git mv` the app files preserving subpaths: `git mv src/core/profiles Apps/GoWToolkit/Source/core/profiles`, `git mv src/core/parsers/gow2 …`, `git mv src/core/parsers/gowr …`, `git mv src/core/loaders …`, `git mv src/core/types/handlers …`, the `core/types/GameTypes.*`+`GameTypeTable.h`, `core/formats`, `core/domain/WadEntryRoleLegacy.h`, `core/WadTypes.h`, the game `ui/*` and game `ui/viewers/*`, `cli/`, `main.cpp`, `AppRegistration.*`. After this `src/` should be EMPTY (verify) — remove it.

- [ ] **Step 2b: Move tests' stubs as needed**

Leave `tests/` where it is but note its include roots will point at both `Engine/Source` and `Apps/GoWToolkit/Source`. The engine-only tests (`logger`,`metrics`,`threading`,`mediakind`,`theme_contrast`,`viewer_registry`) link `Engine`; the golden + table + persistence tests link the app core. (Wiring handled in Step 4.)

- [ ] **Step 3: Write `Engine/CMakeLists.txt`**

```cmake
add_library(Onyx STATIC
    # list Engine/Source/**.cpp (and .mm/.m on APPLE) — enumerate or GLOB_RECURSE
)
target_include_directories(Onyx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(Onyx PUBLIC
    imgui_lib implot_lib imgui_color_text_edit glfw glad glm::glm lz4_lib miniaudio ffmpeg_lib)
# platform libs (opengl32/comdlg32/dwmapi on WIN32; frameworks on APPLE; GL/dl/pthread/m on Linux)
target_compile_features(Onyx PUBLIC cxx_std_20)
add_library(Onyx::Onyx ALIAS Onyx)
```
Move the platform-specific link/compile-def logic (the `if(WIN32)/elseif(APPLE)/else` blocks, the `GLFW_EXPOSE_NATIVE_*`, `NOMINMAX`, `/utf-8`) from the old root `CMakeLists.txt` into here, applied to `Onyx`. The third-party `FetchContent`/`add_library` blocks (imgui, glfw, glm, lz4, glad, miniaudio, implot, ffmpeg, doctest) stay in the ROOT `CMakeLists.txt` (shared).

- [ ] **Step 4: Write `Apps/GoWToolkit/CMakeLists.txt`**

```cmake
add_executable(GoWToolkit
    # list Apps/GoWToolkit/Source/**.cpp (+ .mm/.m on APPLE)
)
target_include_directories(GoWToolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(GoWToolkit PRIVATE Onyx::Onyx)
```
Move the macOS `.app` bundle logic, the FFmpeg-DLL copy, the fonts copy, and `WIN32_EXECUTABLE`/icon handling here (they are per-executable). The app inherits Engine's PUBLIC include (`Engine/Source`) so `#include "core/types/TypeId.h"` resolves; its own `Source` resolves `#include "core/profiles/…"`.

- [ ] **Step 5: Rewrite the root `CMakeLists.txt`**

Keep: `cmake_minimum_required`, `project`, all third-party fetch/build blocks (imgui…ffmpeg…doctest…nlohmann_json). Remove: the old `gowtoolkit_parser_min` lib, the `GLOB_RECURSE src/*.cpp` exe, and all the per-target platform/bundle logic now living in the sub-CMakeLists. Add at the end:
```cmake
add_subdirectory(Engine)
add_subdirectory(Apps/GoWToolkit)
if(GOWTOOLKIT_BUILD_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
```
`gowtoolkit_parser_min` is gone — `tests` now links `Onyx` (engine) plus the specific app `.cpp` it needs (golden tests need the profiles/parsers/handlers/GameTypes). Update `tests/CMakeLists.txt`: the test target links `Onyx` and compiles the app-core sources it exercises (replace the old `gowtoolkit_parser_min` link + the hand-appended `src/core/*.cpp` with `Apps/GoWToolkit/Source/...` paths; the golden harness needs profiles+parsers+handlers+GameTypes+GameTypeTable+AssetVisibilityDefaults). Keep `test_stubs.cpp`/`theme_stubs.cpp` only if still needed after the relink.

- [ ] **Step 6: Configure fresh + build + test**

Use a NEW build dir (the old `build/` caches the old structure):
```
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B build2
cmake --build build2
ctest --test-dir build2 --output-on-failure
```
Expected: configures, builds `Onyx` then `GoWToolkit` then `gowtoolkit_tests`, `100% tests passed`. Work through errors: the common ones are a file assigned to the wrong target (engine file that still references an app symbol → either it was misclassified, move it to App, or it's a real leak to fix), or a missing source in a target's list. **If an "engine" file fails to compile because it needs an app header, that's a boundary violation — do NOT add the app include to the engine; instead reassess: the file belongs in App, or needs a Task1-style decoupling.**

- [ ] **Step 7: Verify the boundary**

```powershell
# No Engine source includes an Apps/ path or names a game symbol:
Select-String -Path (Get-ChildItem -Path Engine/Source -Recurse -Include *.h,*.hpp,*.cpp,*.mm).FullName -Pattern 'Apps/|GameTypes|ProfileGOW|parsers/gow|loaders/GOWR|MaterialViewer|MapViewer|WadBrowser|/handlers/' | Select-Object -First 30
```
Expected: no lines (comments aside). This is the proof the engine is game-agnostic.

- [ ] **Step 8: Point the canonical build dir at the new structure**

Once green, update the dev workflow to use `build2` (or remove `build/` and reconfigure `build/` fresh against the new top-level CMake so the canonical `build/` works again):
```
Remove-Item -Recurse -Force build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build ; ctest --test-dir build --output-on-failure
```
Expected: green from a clean `build/`.

- [ ] **Step 9: Commit**

```
git add Engine Apps tests CMakeLists.txt
git commit -m "refactor(build): Split into Onyx engine library and GoWToolkit app"
```
(This commit is large and full of renames; `git` will detect the `git mv`s as renames. Verify `git show --stat HEAD` shows renames into Engine/ and Apps/, no third_party.)

---

## Verificação final do Plano 3

- [ ] **Step 1: Clean rebuild + full suite from canonical build dir**

`cmake --build build --target clean ; cmake --build build ; ctest --test-dir build --output-on-failure` → green.

- [ ] **Step 2: Engine builds standalone (the real proof)**

`cmake --build build --target Onyx` → the engine library compiles and archives with zero app files. Combined with the Step 7 boundary grep, this proves the SDK is decoupled.

- [ ] **Step 3: Confirm structure**

`src/` no longer exists; `Engine/Source/` and `Apps/GoWToolkit/Source/` hold the split; `Engine/CMakeLists.txt` defines `Onyx` (+ `Onyx::Onyx` alias); `Apps/GoWToolkit/CMakeLists.txt` links it. Plano 3 concluído.

---

## Roadmap dos planos seguintes
- **Plano 4 — PascalCase + public surface + MinimalViewer (M3 + M4).** Rename `Engine/Source/{core,ui,window}` → PascalCase; introduce `Engine/Include/Onyx/` public headers (this is the include-path churn deliberately deferred from M2); build `Apps/MinimalViewer` that registers its own toy types — the second consumer proving the catalog/SDK is truly open.
- **Plano 5 — Physical repo split (M5).** `git filter-repo` extract `Engine/` → `OnyxSDK`; FetchContent; tag `v0.1.0`.

### Carry-forward
- Generalize `WadBrowser`/`Inspector`/`MapViewer` (role/visuals provider interface) — still App after M2.
- `IGameProfile.h` filename still holds `IAssetProfile` — rename during the PascalCase pass (M3).
- Settle the final SDK name before Plano 5 (`Onyx` is placeholder).
