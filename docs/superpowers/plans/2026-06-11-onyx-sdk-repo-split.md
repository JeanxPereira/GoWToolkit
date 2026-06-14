# Onyx SDK — Physical Repo Split (Plano 5: M5) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract the in-repo `Engine/` into its own standalone, history-preserving repository **OnyxSDK** (`github.com/JeanxPereira/OnyxSDK`), make it build on its own with a runnable `MinimalViewer` example, tag it `v0.1.0`, and rewire GoWToolkit to consume it back via CMake `FetchContent` on that tag — leaving GoWToolkit's golden tests green. After this, a new app (e.g. SCUMMRedux) can depend on OnyxSDK without GoWToolkit.

**Architecture:** Two parts, each ending in a green, committed state. **Part A** builds OnyxSDK: `git filter-repo --subdirectory-filter Engine` on a clone lifts `Engine/Source`→`Source`, `Engine/Include`→`Include` with their history (back to the M2 split that created `Engine/`); then the third-party dependency machinery — which today lives in GoWToolkit's *root* `CMakeLists.txt`, not in `Engine/` — is reconstructed in a new OnyxSDK root `CMakeLists.txt` (deps + the `Onyx` static lib + `Onyx::Onyx` alias), the dep submodules/vendored sources/fonts are brought across, `MinimalViewer` moves to `Examples/MinimalViewer`, the engine-only unit tests move to `Tests/`, repo metadata is added, it builds standalone, and it is pushed + tagged `v0.1.0`. **Part B** rewires GoWToolkit: delete `Engine/` and `Apps/MinimalViewer`, strip the dep-fetching blocks from the root `CMakeLists.txt`, add `FetchContent_Declare(OnyxSDK GIT_TAG v0.1.0)` + `MakeAvailable`, and relink the app + tests against the inherited `Onyx::Onyx` (which transitively provides imgui/glfw/glm/lz4/miniaudio/ffmpeg and exports `FFMPEG_DLLS`).

**Tech Stack:** C++20, CMake + Ninja + FetchContent, MSVC, `git filter-repo`, GitHub (`gh`). Golden GOW2/GOWR snapshot tests remain the regression net for GoWToolkit; the engine-only unit tests + `MinimalViewer_SelfTest` are the net for OnyxSDK.

**Scope:** ONLY M5 — the physical split + FetchContent consumption + tag v0.1.0. Out of scope: `install()`/`find_package(Onyx)` export (YAGNI until a third consumer needs it — FetchContent suffices); generalizing `WadBrowser`/`Inspector`/`MapViewer`; the pre-publish minor follow-ups (injectable file-dialog filter, IAssetProfile comment) — keep as carry-forward; SCUMMRedux itself.

**Decisions locked (from planning):**
- **Name:** `Onyx` is now permanent — NO rename. Repo name `OnyxSDK`.
- **Hosting/consumption:** real GitHub repo `github.com/JeanxPereira/OnyxSDK`; GoWToolkit consumes via `FetchContent` `GIT_TAG v0.1.0`.
- **History:** preserved via `git filter-repo --subdirectory-filter Engine`. Tradeoff (accepted): OnyxSDK history begins where `Engine/` was created (the M2 split commit `52b4961`); pre-split `src/` history — when engine and app files were mixed — is not carried into OnyxSDK. The M3a/M3b/M3c/M4 work (all under `Engine/`) IS preserved.

---

## Environment (this machine)

`cl` is NOT on PATH. Enter the VS 2022 BuildTools dev shell IN THE SAME shell call as any build/test:
```powershell
Import-Module "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools" -DevCmdArguments "-arch=x64 -host_arch=x64" -SkipAutomaticLocation
```
Ignore the benign `vswhere.exe` warning. `cmake`/`ninja` on PATH. GoWToolkit canonical build dir `build/`.

**`git filter-repo`:** verify it is installed (`git filter-repo --version`); if absent, `pip install git-filter-repo` (it is a single Python script on PATH as `git-filter-repo`). filter-repo refuses to run on a repo that is not a fresh clone unless `--force`; we always run it on a throwaway clone, never on the working repo.

**`gh`:** the GitHub CLI token in the keyring is currently INVALID (see Pré-requisito Step 3). `git push` uses git's own credential helper and works; `gh` API calls may not. Creating the remote repo can be done via `gh repo create` (needs working `gh` auth) OR manually in the browser — the plan provides both.

**Commit hygiene (MANDATORY):** `git add` EXPLICIT paths only — **never `git add -A`** (the GoWToolkit `third_party/` submodules carry uncommitted deltas that must stay out of GoWToolkit commits). **No `Co-Authored-By` / "Generated with" / AI-credit trailers** in any commit or PR (standing user instruction). Never leave a red tree at a checkpoint.

**Paths:** GoWToolkit = `C:\CodingProjects\Personal\GoWToolkit`. OnyxSDK will be created at the sibling path `C:\CodingProjects\Personal\OnyxSDK`. The throwaway filter clone uses `C:\CodingProjects\Personal\OnyxSDK-filter-tmp`.

---

## Pré-requisito: baseline + tooling + remote

- [ ] **Step 1: Green baseline on `feat/onyx-genericization`**

In GoWToolkit (dev shell, one call): `cmake --build build ; ctest --test-dir build --output-on-failure` → expect `100% tests passed` (8: unit, Golden_GOW2, Golden_GOWR, Metrics, Logger, Threading, ThemeContrast, MinimalViewer_SelfTest). If not green, STOP.

- [ ] **Step 2: Confirm `git filter-repo` is available**

```powershell
git filter-repo --version
```
If "not a git command", install: `pip install git-filter-repo` then re-check. Do not proceed without it.

- [ ] **Step 3: Create the empty GitHub repo `OnyxSDK`**

Try the CLI first:
```powershell
gh repo create JeanxPereira/OnyxSDK --public --description "Onyx — a reusable game-asset-explorer SDK" --disable-wiki
```
If `gh` fails (invalid keyring token: run `gh auth refresh -h github.com` interactively first, OR) create the repo manually at https://github.com/new (owner `JeanxPereira`, name `OnyxSDK`, Public, NO README/license/gitignore — must be empty so the filtered history pushes cleanly). Confirm the empty repo exists before Part A Step "push".

---

# Part A — Build the OnyxSDK repository

## Task A1: Extract `Engine/` with history (filter-repo)

**Files:** none in GoWToolkit are modified here — this operates on a throwaway clone and produces `C:\CodingProjects\Personal\OnyxSDK`.

- [ ] **Step 1: Fresh clone of GoWToolkit for filtering**

```powershell
cd C:\CodingProjects\Personal
git clone C:\CodingProjects\Personal\GoWToolkit OnyxSDK-filter-tmp
cd OnyxSDK-filter-tmp
git checkout feat/onyx-genericization
```
(Clone from the LOCAL path so all branch history is present; we filter the current branch.)

- [ ] **Step 2: Subdirectory-filter to lift `Engine/` to root**

```powershell
git filter-repo --subdirectory-filter Engine --force
```
After this, the repo root contains `Source/`, `Include/`, and the engine's `CMakeLists.txt` (the `add_library(Onyx ...)` one) — each with the history of every commit that touched `Engine/`. Verify:
```powershell
Get-ChildItem            # expect: CMakeLists.txt, Include/, Source/
git log --oneline | Select-Object -First 12   # expect the M3a..M4 engine commits, paths now rooted at Source/Include
```

- [ ] **Step 3: Move the filtered tree into the real OnyxSDK working dir**

```powershell
cd C:\CodingProjects\Personal
Rename-Item OnyxSDK-filter-tmp OnyxSDK
cd OnyxSDK
git remote remove origin   # was pointing at the local GoWToolkit clone; OnyxSDK gets its own remote later
```
The extracted engine `CMakeLists.txt` at the OnyxSDK root is the LIBRARY definition (`add_library(Onyx STATIC ...)`, `Onyx::Onyx` alias, dep links, platform flags). It is NOT yet a top-level `project()`; Task A3 rewrites it into a proper root that fetches deps first. Keep it for reference — Task A3 reuses its `add_library`/link/flags blocks verbatim.

## Task A2: Bring the third-party dependencies into OnyxSDK

The engine links `imgui_lib implot_lib imgui_color_text_edit glfw glad glm::glm lz4_lib miniaudio ffmpeg_lib`. Of these: `glfw`, `glm`, `lz4`, `doctest` are pure FetchContent (no checked-in source — nothing to copy, the OnyxSDK root CMake will fetch them); `imgui` is FetchContent with a checked-in SOURCE_DIR; `implot` and `imgui_color_text_edit` are git submodules; `glad` and `miniaudio` are vendored checked-in sources; `fonts` are a runtime asset the engine's FontManager loads (needed by the MinimalViewer example). `bcdec` is app-only (used by GoWToolkit's image loader, NOT the engine) — do NOT copy it.

**Files:** create `OnyxSDK/third_party/` with the engine's vendored + submodule deps; create `OnyxSDK/.gitmodules`.

- [ ] **Step 1: Copy vendored sources + fonts from GoWToolkit**

```powershell
New-Item -ItemType Directory -Force C:\CodingProjects\Personal\OnyxSDK\third_party | Out-Null
Copy-Item -Recurse C:\CodingProjects\Personal\GoWToolkit\third_party\glad      C:\CodingProjects\Personal\OnyxSDK\third_party\glad
Copy-Item -Recurse C:\CodingProjects\Personal\GoWToolkit\third_party\miniaudio C:\CodingProjects\Personal\OnyxSDK\third_party\miniaudio
Copy-Item -Recurse C:\CodingProjects\Personal\GoWToolkit\third_party\fonts     C:\CodingProjects\Personal\OnyxSDK\third_party\fonts
Copy-Item -Recurse C:\CodingProjects\Personal\GoWToolkit\third_party\imgui     C:\CodingProjects\Personal\OnyxSDK\third_party\imgui
```
(`imgui` is copied so the OnyxSDK `FetchContent_Declare(imgui ... SOURCE_DIR third_party/imgui)` resolves offline exactly as GoWToolkit's does. Do not copy `.git` metadata if present inside — it is a plain source tree.)

- [ ] **Step 2: Add `implot` and `imgui_color_text_edit` as submodules in OnyxSDK**

```powershell
cd C:\CodingProjects\Personal\OnyxSDK
git submodule add https://github.com/epezent/implot.git third_party/implot
git submodule add https://github.com/goossens/ImGuiColorTextEdit.git third_party/imgui_color_text_edit
git submodule update --init --recursive
```
This writes `OnyxSDK/.gitmodules`. Verify both dirs are populated (non-empty).

## Task A3: Write the OnyxSDK root `CMakeLists.txt`

Compose a proper top-level project that (1) fetches/builds all deps exactly as GoWToolkit's root does today, then (2) defines the `Onyx` library (reuse the extracted engine cmake's `add_library`/`target_*` blocks), then (3) adds the example and tests.

**Files:** overwrite `OnyxSDK/CMakeLists.txt` (the filter-extracted engine cmake) with the new root; create `OnyxSDK/cmake/` only if you choose to factor helpers (optional).

- [ ] **Step 1: Author the root CMakeLists**

Structure (transcribe the concrete dep blocks from `GoWToolkit/CMakeLists.txt` — they are the authoritative, working versions; do not paraphrase the FetchContent declarations, copy them):
```cmake
cmake_minimum_required(VERSION 3.20)
project(OnyxSDK LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(ONYX_BUILD_EXAMPLES "Build the Onyx example apps" ON)
option(ONYX_BUILD_TESTS    "Build the Onyx engine unit tests" ON)

include(FetchContent)

# ── Third-party deps — COPY VERBATIM the corresponding blocks from
#    GoWToolkit/CMakeLists.txt: imgui (SOURCE_DIR third_party/imgui),
#    glfw 3.3.9, glm 1.0.1, lz4 1.9.4 (+ the manual lz4_lib static target),
#    glad (manual static lib), miniaudio (manual static lib),
#    implot (FetchContent_Populate from third_party/implot + implot_lib),
#    imgui_color_text_edit (manual static lib from the submodule),
#    FFmpeg (Windows BtbN prebuilt download → ffmpeg_lib INTERFACE + FFMPEG_DLLS;
#            Linux/macOS pkg-config), and (only if ONYX_BUILD_TESTS) doctest 2.4.11
#    + nlohmann_json 3.11.3.
#    Keep the imgui_lib / implot_lib / lz4_lib / glad / miniaudio target names
#    identical so the Onyx target's link list is unchanged. ──
# ... (verbatim dep blocks here) ...

# ── Export FFMPEG_DLLS to consumers (FetchContent runs this file in a child
#    scope; a plain variable would not reach GoWToolkit). Make it CACHE so a
#    parent project that FetchContent's OnyxSDK can read it for its DLL copy. ──
if(WIN32 AND FFMPEG_DLLS)
    set(ONYX_FFMPEG_DLLS "${FFMPEG_DLLS}" CACHE INTERNAL "FFmpeg runtime DLLs for Onyx consumers")
endif()

# ── The Onyx engine library (reuse the extracted Engine/CMakeLists.txt body) ──
file(GLOB_RECURSE ONYX_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")
if(APPLE)
    file(GLOB_RECURSE ONYX_OBJC CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.mm" "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.m")
    list(APPEND ONYX_SOURCES ${ONYX_OBJC})
endif()
add_library(Onyx STATIC ${ONYX_SOURCES})
add_library(Onyx::Onyx ALIAS Onyx)
target_include_directories(Onyx
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_features(Onyx PUBLIC cxx_std_20)
target_link_libraries(Onyx PUBLIC
    imgui_lib implot_lib imgui_color_text_edit glfw glad glm::glm lz4_lib miniaudio ffmpeg_lib)
if(MSVC)
    target_compile_options(Onyx PUBLIC /utf-8)
endif()
target_compile_definitions(Onyx PUBLIC
    $<$<PLATFORM_ID:Windows>:GLFW_EXPOSE_NATIVE_WIN32>
    $<$<PLATFORM_ID:Darwin>:GLFW_EXPOSE_NATIVE_COCOA>
    $<$<PLATFORM_ID:Windows>:GOWTOOL_OS_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:GOWTOOL_OS_MACOS>
    $<$<PLATFORM_ID:Linux>:GOWTOOL_OS_LINUX>
    $<$<PLATFORM_ID:Windows>:NOMINMAX>)
if(WIN32)
    target_link_libraries(Onyx PUBLIC opengl32 comdlg32 dwmapi)
elseif(APPLE)
    target_link_libraries(Onyx PUBLIC "-framework OpenGL" "-framework AudioToolbox"
        "-framework CoreAudio" "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation")
else()
    target_link_libraries(Onyx PUBLIC GL dl pthread m)
endif()

# ── Provide the engine's runtime font dir to consumers as a known path ──
set(ONYX_FONTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts" CACHE INTERNAL "Onyx bundled fonts")

if(ONYX_BUILD_EXAMPLES)
    add_subdirectory(Examples/MinimalViewer)
endif()
if(ONYX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
```

> When copying the dep blocks, replace any `${CMAKE_SOURCE_DIR}` / `third_party/...` paths so they resolve relative to the OnyxSDK root (`${CMAKE_CURRENT_SOURCE_DIR}`). The FFmpeg Windows block sets `FFMPEG_DLLS` from the downloaded `bin/*.dll` — keep that, then mirror it into `ONYX_FFMPEG_DLLS` CACHE as shown.

## Task A4: Move `MinimalViewer` into `Examples/`

**Files:** move `GoWToolkit/Apps/MinimalViewer/**` → `OnyxSDK/Examples/MinimalViewer/**`; adjust its CMake for the new context.

- [ ] **Step 1: Copy MinimalViewer sources into OnyxSDK**

```powershell
New-Item -ItemType Directory -Force C:\CodingProjects\Personal\OnyxSDK\Examples | Out-Null
Copy-Item -Recurse C:\CodingProjects\Personal\GoWToolkit\Apps\MinimalViewer C:\CodingProjects\Personal\OnyxSDK\Examples\MinimalViewer
```
(The GoWToolkit copy is deleted in Part B. Copy now so OnyxSDK is self-contained for its standalone build.)

- [ ] **Step 2: Adjust `Examples/MinimalViewer/CMakeLists.txt` for OnyxSDK paths**

In the copied `OnyxSDK/Examples/MinimalViewer/CMakeLists.txt`: the `target_link_libraries(MinimalViewer PRIVATE Onyx::Onyx)` stays. Replace any `${CMAKE_SOURCE_DIR}/third_party/fonts` font-copy path with `${ONYX_FONTS_DIR}`, and any `FFMPEG_DLLS` reference with `${ONYX_FFMPEG_DLLS}` (the CACHE vars from Task A3). Example POST_BUILD copies:
```cmake
if(NOT APPLE)
    add_custom_command(TARGET MinimalViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ONYX_FONTS_DIR}" "$<TARGET_FILE_DIR:MinimalViewer>/third_party/fonts"
        COMMENT "Copying fonts next to MinimalViewer")
endif()
if(WIN32 AND ONYX_FFMPEG_DLLS)
    add_custom_command(TARGET MinimalViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ONYX_FFMPEG_DLLS} "$<TARGET_FILE_DIR:MinimalViewer>"
        COMMENT "Copying FFmpeg DLLs next to MinimalViewer")
endif()
```

## Task A5: Engine-only tests + repo metadata

**Files:** create `OnyxSDK/Tests/` with the truly engine-only unit tests; add `README.md`, `LICENSE`, `CHANGELOG.md`, `CMakePresets.json`, `.gitignore`.

- [ ] **Step 1: Bring the engine-only unit tests**

Copy ONLY the tests that link `Onyx::Onyx` with NO app-core sources: `sanity_test.cpp`, `logger_test.cpp`, `metrics_test.cpp`, `threading_test.cpp`, `theme_contrast_test.cpp`, plus the doctest `main.cpp`. (EXCLUDE `golden_gow2/gowr`, `golden_helpers`, `typetable_equivalence_test`, `visibility_persistence_test`, `mediakind_test`, `viewer_registry_test`, `test_stubs.cpp` — those compile GoWToolkit app-core sources/`GameTypes` and stay in GoWToolkit.)
```powershell
New-Item -ItemType Directory -Force C:\CodingProjects\Personal\OnyxSDK\Tests | Out-Null
foreach ($t in 'sanity_test.cpp','logger_test.cpp','metrics_test.cpp','threading_test.cpp','theme_contrast_test.cpp','main.cpp') {
  Copy-Item "C:\CodingProjects\Personal\GoWToolkit\tests\$t" "C:\CodingProjects\Personal\OnyxSDK\Tests\$t"
}
```
Create `OnyxSDK/Tests/CMakeLists.txt`:
```cmake
add_executable(onyx_tests
    main.cpp sanity_test.cpp logger_test.cpp metrics_test.cpp
    threading_test.cpp theme_contrast_test.cpp)
target_link_libraries(onyx_tests PRIVATE Onyx::Onyx doctest::doctest imgui_lib)
add_test(NAME OnyxUnit       COMMAND onyx_tests)
add_test(NAME OnyxLogger     COMMAND onyx_tests "--test-case=*Logger*")
add_test(NAME OnyxMetrics    COMMAND onyx_tests "--test-case=*Metrics*")
add_test(NAME OnyxThreading  COMMAND onyx_tests "--test-case=*Threading*")
add_test(NAME OnyxThemeContrast COMMAND onyx_tests "--test-case=*ThemeContrast*")
```
> If `theme_contrast_test.cpp` or any kept test still references an app symbol at link time, drop it from the OnyxSDK test set (it was not as engine-pure as the recon implied) and note it — do NOT pull app sources into OnyxSDK.

- [ ] **Step 2: Repo metadata**

Create `OnyxSDK/README.md` (what Onyx is, how to consume via FetchContent, how to build the example), `OnyxSDK/LICENSE` (match GoWToolkit's license — copy it), `OnyxSDK/CHANGELOG.md` (a `## v0.1.0` entry summarising the extraction), `OnyxSDK/.gitignore` (`build/`, `Build/`, `build-*/`, IDE dirs), and `OnyxSDK/CMakePresets.json` (a `debug`/`release` Ninja preset pair). Keep these short and factual.

## Task A6: Build OnyxSDK standalone, push, tag

- [ ] **Step 1: Configure + build + test OnyxSDK on its own**

```powershell
cd C:\CodingProjects\Personal\OnyxSDK
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
Expected: `Onyx` archives, `MinimalViewer` links (only `Onyx`), `onyx_tests` runs, and `MinimalViewer_SelfTest` (if wired into Tests — optionally add `add_test(NAME MinimalViewer_SelfTest COMMAND MinimalViewer --selftest $<TARGET_FILE:MinimalViewer>)` referencing the example target) passes. Work through any missing-dep errors (a dep block not copied, a path not rebased to `CMAKE_CURRENT_SOURCE_DIR`). This is the proof OnyxSDK stands alone.

- [ ] **Step 2: Commit the OnyxSDK additions (deps wiring, examples, tests, meta)**

The filter-repo history is already present; stage the NEW files (root CMakeLists, third_party additions, Examples, Tests, metadata, .gitmodules) and commit:
```powershell
cd C:\CodingProjects\Personal\OnyxSDK
git add CMakeLists.txt third_party .gitmodules Examples Tests README.md LICENSE CHANGELOG.md .gitignore CMakePresets.json
git commit -m "build(onyx): Standalone OnyxSDK — deps, MinimalViewer example, engine tests"
```

- [ ] **Step 3: Push to GitHub and tag v0.1.0**

```powershell
cd C:\CodingProjects\Personal\OnyxSDK
git remote add origin https://github.com/JeanxPereira/OnyxSDK.git
git branch -M main
git push -u origin main
git tag -a v0.1.0 -m "Onyx SDK v0.1.0 — extracted from GoWToolkit"
git push origin v0.1.0
```
Verify the tag is on the remote: `git ls-remote --tags origin` shows `v0.1.0`. (FetchContent in Part B resolves this exact tag.)

**Part A checkpoint:** OnyxSDK builds standalone (engine + example + unit tests green), is pushed to `github.com/JeanxPereira/OnyxSDK`, and tagged `v0.1.0`.

---

# Part B — Rewire GoWToolkit to consume OnyxSDK

All remaining work is in the GoWToolkit repo on `feat/onyx-genericization`.

## Task B1: Remove the now-external engine + example

**Files:** delete `GoWToolkit/Engine/`, delete `GoWToolkit/Apps/MinimalViewer/`, strip dep-fetching + `add_subdirectory(Engine)`/`add_subdirectory(Apps/MinimalViewer)` from `GoWToolkit/CMakeLists.txt`.

- [ ] **Step 1: Delete Engine and MinimalViewer from GoWToolkit**

```powershell
cd C:\CodingProjects\Personal\GoWToolkit
git rm -r Engine Apps/MinimalViewer
```
(They now live in OnyxSDK. The app's vendored `third_party/bcdec` and `third_party/fonts` STAY — bcdec is app-only; fonts: see Task B2 Step 3.)

## Task B2: Consume OnyxSDK via FetchContent

**Files:** modify `GoWToolkit/CMakeLists.txt`, `GoWToolkit/Apps/GoWToolkit/CMakeLists.txt`, `GoWToolkit/tests/CMakeLists.txt`.

- [ ] **Step 1: Replace the dep-fetching blocks with a single FetchContent(OnyxSDK)**

In `GoWToolkit/CMakeLists.txt`, REMOVE all the third-party `FetchContent_Declare`/`FetchContent_MakeAvailable`/manual-lib blocks for imgui, glfw, glm, lz4, glad, miniaudio, implot, imgui_color_text_edit, and the FFmpeg download block (these are now provided transitively by OnyxSDK). KEEP `doctest` + `nlohmann_json` (the GoWToolkit golden tests need them and OnyxSDK only fetches them under its own `ONYX_BUILD_TESTS`, which we disable for the consumer). KEEP `bcdec` (app-only). Then add:
```cmake
include(FetchContent)
FetchContent_Declare(OnyxSDK
    GIT_REPOSITORY https://github.com/JeanxPereira/OnyxSDK.git
    GIT_TAG        v0.1.0)
set(ONYX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # consumer doesn't build Onyx's example
set(ONYX_BUILD_TESTS    OFF CACHE BOOL "" FORCE)   # consumer doesn't build Onyx's unit tests
FetchContent_MakeAvailable(OnyxSDK)
# Onyx::Onyx + imgui_lib/glfw/glm::glm/lz4_lib/miniaudio/ffmpeg_lib + ONYX_FFMPEG_DLLS now available.
```
Keep `add_subdirectory(Apps/GoWToolkit)` and the tests wiring; REMOVE `add_subdirectory(Engine)` and `add_subdirectory(Apps/MinimalViewer)`.

> **Local-dev override (optional, document in a comment):** to iterate against the local OnyxSDK without re-tagging, configure GoWToolkit with `-DFETCHCONTENT_SOURCE_DIR_ONYXSDK=C:/CodingProjects/Personal/OnyxSDK`. CI / clean builds use the pinned `GIT_TAG v0.1.0`.

- [ ] **Step 2: Relink the app target**

`Apps/GoWToolkit/CMakeLists.txt` already does `target_link_libraries(GoWToolkit PRIVATE Onyx::Onyx)` and `target_include_directories(... third_party/bcdec)`. Confirm it still resolves (Onyx::Onyx now comes from FetchContent). Replace any `FFMPEG_DLLS` reference in its POST_BUILD DLL copy with `${ONYX_FFMPEG_DLLS}`. Its font copy uses `${CMAKE_SOURCE_DIR}/third_party/fonts` — see Step 3.

- [ ] **Step 3: Resolve the fonts source for GoWToolkit**

GoWToolkit's `third_party/fonts` still exists (not removed). Two valid options — pick the simpler: **(a)** keep GoWToolkit copying its OWN `third_party/fonts` (leave its POST_BUILD as-is) — fonts are duplicated across repos but each app is self-sufficient; **(b)** point GoWToolkit's font copy at `${ONYX_FONTS_DIR}` (the SDK's fonts) and `git rm -r GoWToolkit/third_party/fonts`. Recommended: **(a)** — least churn, no cross-repo runtime-asset coupling. Document the choice in a comment.

- [ ] **Step 4: Fix the test target's engine include + link**

`tests/CMakeLists.txt` currently adds `${CMAKE_SOURCE_DIR}/Engine/Source` to the test include dirs and may reference `Engine/Source` paths. Since the engine is now an external target, the tests must include the engine ONLY via the inherited PUBLIC `Onyx::Onyx` interface (`<Onyx/...>`). REMOVE any `Engine/Source` include-dir line; ensure `onyx`-relative includes in the test sources are `<Onyx/...>` (they should already be post-M3c). The test target keeps compiling the GoWToolkit app-core sources (`Apps/GoWToolkit/Source/core/...`) and linking `Onyx::Onyx doctest::doctest nlohmann_json`. The `test_stubs.cpp` GOWR/`GetTexIndex` stubs stay.

## Task B3: Build GoWToolkit against OnyxSDK + commit

- [ ] **Step 1: Clean reconfigure (FetchContent must clone OnyxSDK) + build + test**

A fresh build dir forces FetchContent to fetch the tag:
```powershell
Remove-Item -Recurse -Force build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
Expected: FetchContent clones `OnyxSDK@v0.1.0`, builds `Onyx` + deps, then `GoWToolkit` + `gowtoolkit_tests`; `100% tests passed` (7 now — the engine-only `MinimalViewer_SelfTest` moved to OnyxSDK; the remaining are unit/Golden_GOW2/Golden_GOWR/Metrics/Logger/Threading/ThemeContrast). Work through fetch/link errors: a dep the app used directly that OnyxSDK doesn't re-export PUBLIC (promote that link in OnyxSDK and re-tag → bump to `v0.1.1`, or link it in GoWToolkit if it is genuinely app-side).

> If the golden tests fail because the relinked test target can't see an engine header it previously got via the in-tree `Engine/Source` include root, that header was being consumed as a PRIVATE engine header by the test — promote it into `Include/Onyx/` in OnyxSDK, re-tag, and re-point the GIT_TAG. (This is the same "promote on use" rule from M3c, now across the repo boundary.)

- [ ] **Step 2: Commit the consumption rewire**

```powershell
git add CMakeLists.txt Apps/GoWToolkit/CMakeLists.txt tests/CMakeLists.txt Engine Apps/MinimalViewer
git commit -m "build(onyx): Consume OnyxSDK via FetchContent; drop in-tree engine"
```
(Staging `Engine`/`Apps/MinimalViewer` records their deletion. Verify `git show --stat HEAD` shows the removals + the three CMake edits, no `third_party` deltas.)

**Part B checkpoint:** GoWToolkit builds by fetching `OnyxSDK@v0.1.0`; golden GOW2/GOWR green; no in-tree `Engine/`.

---

## Verificação final do Plano 5

- [ ] **Step 1: Two-repo clean build**

OnyxSDK (`cmake -B build ; cmake --build build ; ctest --test-dir build --output-on-failure`) green standalone. GoWToolkit from a fresh `build/` (forces the FetchContent fetch) green with golden GOW2/GOWR.

- [ ] **Step 2: Decoupling proof**

GoWToolkit's `CMakeLists.txt` contains no `add_subdirectory(Engine)` and no engine source; the only path to the engine is `FetchContent ... OnyxSDK ... GIT_TAG v0.1.0`. OnyxSDK builds + tests + runs its `MinimalViewer` example with zero GoWToolkit code. **A new app can now depend on OnyxSDK alone — SCUMMRedux can be born.**

- [ ] **Step 3: Push GoWToolkit**

`git push origin feat/onyx-genericization` (updates PR #1 with the consumption rewire). Confirm the push succeeded.

---

## Carry-forward (post-Plano-5)
- Pre-publish minors from the M4 review: make `SystemOpenFileDialog` filter injectable (engine currently hardcodes `*.wad;*.iso;*.pak`); de-leak the `IAssetProfile::PrepareForParse` comment; fix pre-existing `InvalidateLodIndex` not resetting `s_texIndexStarted` in GOWR loaders.
- `install()` + `OnyxConfig.cmake` / `find_package(Onyx)` export — add only when a third app needs binary consumption (FetchContent suffices for now).
- Generalize `WadBrowser`/`Inspector`/`MapViewer` via a role/visuals-provider interface (still App-side).
- Prune leftover agent worktrees under `GoWToolkit/.claude/worktrees/` and the stale `feat/onyx-genericization-work` branch.
- Bootstrap **SCUMMRedux** as a second real OnyxSDK consumer (the original motivation).

---

## Self-review notes (coverage vs design spec §3 Fase 2 / §4 M5)
- **filter-repo extract Engine/ → OnyxSDK (history preserved):** Task A1. ✓ (tradeoff on pre-split history documented).
- **OnyxSDK self-contained deps/build:** Tasks A2–A3 (the dep machinery that lived in GoWToolkit's root, reconstructed in OnyxSDK's root — the spec's diagram implied this but didn't call it out; surfaced by recon). ✓
- **MinimalViewer → Examples/ (spec §3 Fase 2 tree):** Task A4. ✓
- **GoWToolkit consumes via FetchContent on a tag (spec §3 consumo):** Tasks B1–B3. ✓
- **Tag OnyxSDK v0.1.0:** Task A6 Step 3. ✓
- **Golden green throughout / boundary CI rule:** every checkpoint builds + golden. ✓
- **Deferred per spec non-objectives (find_package/install):** carry-forward, not done. ✓
