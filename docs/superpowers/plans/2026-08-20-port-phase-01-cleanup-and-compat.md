# Port Phases 0+1 — Cleanup and Mechanical Compatibility

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clear the encoding damage and the false documentation, then move every mechanical Onyx v0.6.0 → v1.1 rename, so the phases that follow contain only real design work.

**Architecture:** Nothing here changes program semantics. Phase 0 is bytes and prose; Phase 1 is renames plus one umbrella-header alias that keeps 245 call sites compiling untouched. The app target does not build at the start (it calls `SceneData::pbrLayers`, which exists in no published Onyx tag) and is not expected to build at the end either — Phase 2 and 3 do that. The gate throughout is the test target, which does build.

**Tech Stack:** C++20, CMake + Ninja, **MSVC** (see Global Constraints — this matters), doctest, CTest.

**Spec:** `docs/superpowers/specs/2026-08-20-gowtoolkit-onyx-v1-port-design.md`

## Global Constraints

- **Build with MSVC, not GCC.** A bare `cmake` on this machine resolves to the Strawberry Perl GCC on PATH and fails inside Onyx itself (`DWMWA_USE_IMMERSIVE_DARK_MODE` undeclared). Configure and build from a shell that has imported `vcvars64.bat`:
  ```
  "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat"
  ```
  The build directory for this work is `build-msvc/` (already configured). `build/` is a stale GCC configure — ignore it.
- **The test baseline is 5/7 and must not get worse.** Passing: `Golden_GOW2`, `Golden_GOWR`, `Metrics`, `Logger`, `Threading`. Failing (pre-existing, do NOT try to fix in this plan): `unit`, `ThemeContrast` — both from `tests/theme_contrast_test.cpp:94` asserting dark-surface luminance ≤ 0.35 against Onyx values of 0.38–0.55.
- **`Golden_GOW2` and `Golden_GOWR` are the regression gate.** They parse real truncated WADs against pinned JSON snapshots. Nothing in this plan should change what the parsers produce. If either goes red, stop.
- **Never regenerate a golden to make a test pass.**
- **Do not touch `docs/GoW1/Formats/`, `docs/GoW2/Formats/`, or `docs/GoWRknk/Formats/`** — 48 documents of irreplaceable format knowledge, and they are *not* mojibake-damaged.
- Commits: Conventional Commits, sentence-case subject, no trailing period. NEVER add any AI/Claude attribution or co-authorship trailer. Stage explicit paths — never `git add -A` or `git add .`; `third_party/` carries uncommitted deltas.

---

### Task 1: Strip the BOMs

76 files begin with a UTF-8 BOM (`EF BB BF`). The set is a strict superset of the mojibake set — same editor round-trip caused both — so clearing BOMs first gives a clean marker for what the bad tool touched.

**Files:** 76 under `Source/` and `tests/`. Zero under `docs/`, `cmake/`, or the root `CMakeLists.txt`.

**Interfaces:** none — byte-level only.

- [ ] **Step 1: Capture the before-state**

```bash
git rev-parse HEAD > /tmp/port-p0-base.txt
find Source tests -type f \( -name '*.cpp' -o -name '*.h' \) -exec sh -c \
  'head -c3 "$1" | od -An -tx1 | grep -q "ef bb bf" && echo "$1"' _ {} \; | sort > /tmp/bom-before.txt
wc -l /tmp/bom-before.txt
```
Expected: 76.

- [ ] **Step 2: Strip them**

```bash
while read -r f; do
  tail -c +4 "$f" > "$f.tmp" && mv "$f.tmp" "$f"
done < /tmp/bom-before.txt
```

- [ ] **Step 3: Verify only bytes changed, not content**

```bash
find Source tests -type f \( -name '*.cpp' -o -name '*.h' \) -exec sh -c \
  'head -c3 "$1" | od -An -tx1 | grep -q "ef bb bf" && echo "$1"' _ {} \; | wc -l
git diff --stat | tail -3
```
Expected: zero BOMs remain; the diff touches 76 files with 1 changed line each (the first line).

- [ ] **Step 4: Prove the build and tests are unmoved**

```
cmake --build build-msvc --target gowtoolkit_tests
ctest --test-dir build-msvc
```
Expected: builds; 5/7 pass with exactly `unit` and `ThemeContrast` failing.

- [ ] **Step 5: Commit**

```bash
git add Source tests
git commit -m "style: Strip UTF-8 BOMs from source and tests"
```

---

### Task 2: Repair the mojibake

46 files carry cp1252-misdecoded UTF-8 in comments. Six are doubly corrupted (corrupted, saved, corrupted again): `Source/cli/CliApp.cpp`, `Source/core/types/handlers/ContentHandlers.cpp`, `Source/core/types/handlers/InstanceHandler.cpp`, `Source/core/types/handlers/StructuralHandlers.cpp`, `Source/ui/Inspector.cpp`, `Source/ui/WadBrowser.cpp`.

**The danger to avoid:** `docs/` contains correctly-encoded Portuguese (`ção`, `código`, `pós`). A naive `Ã`-hunting regex would corrupt it. Restrict every operation to `Source/` and `tests/`, and match on the unambiguous double-encode signatures only.

**Interfaces:** none — comments only. No code identifier should change.

- [ ] **Step 1: Inventory before touching anything**

```bash
grep -rlP '\xc3\xa2\xe2|\xc3\x83\xc2' Source tests | sort > /tmp/mojibake-before.txt
wc -l /tmp/mojibake-before.txt
grep -rlP '\xc3\xa2\xe2|\xc3\x83\xc2' docs | wc -l
```
Expected: 46 in Source/tests; **0 in docs**. If docs is non-zero, stop and report — the assumption behind this task is wrong.

- [ ] **Step 2: Fix the double-encoded generation first**

Order matters: the triple-pass corruption contains the single-pass pattern inside it, so repairing single-pass first would leave the doubly-corrupted files in a half-repaired state. Handle `\xc3\x83\xc2...` sequences first, then `\xc3\xa2\xe2\x82\xac...`.

The common substitutions, from the observed corpus (all in comments, mostly box-drawing banners and em dashes):

| Corrupt bytes | Renders as | Should be |
|---|---|---|
| `C3 A2 E2 82 AC E2 80 9D` | `â€"` | `—` (U+2014 em dash) |
| `C3 A2 E2 82 AC E2 80 9C` | `â€œ` | `"` (U+201C) |
| `C3 A2 E2 82 AC C2 A6` | `â€¦` | `…` |
| `C3 83 C2 A2 C3 A2 E2 82 AC ...` | `Ã¢â‚¬â€` | (double-decode, then as above) |
| `C3 A7 C3 A3 6F` in `definiÃ§Ã£o` | `Ã§Ã£` | `çã` |

Work file by file rather than with one tree-wide `sed`: each file's damage is small (1–53 hits) and a wrong global substitution is hard to unpick. Read each hit in context before replacing — several are inside box-drawing banner comments where the intended character is `─` or `│`, not an em dash.

Two files carry genuine Portuguese comments that are also corrupted and must be restored to real Portuguese, not to ASCII:
- `Source/core/parsers/gow2/MaterialParser.cpp:8` — `definiÃ§Ã£o` → `definição`
- `Source/core/profiles/gow2/formats/MDL.h:11` — same pattern

- [ ] **Step 3: Verify the repair is total and confined**

```bash
grep -rlP '\xc3\xa2\xe2|\xc3\x83\xc2' Source tests | wc -l
git diff --stat -- docs | wc -l
```
Expected: 0 remaining; **0 lines of diff under `docs/`**.

- [ ] **Step 4: Verify no identifier changed**

```bash
git diff -U0 -- Source tests | grep -E '^[+-]' | grep -vE '^[+-]{3}' | grep -vE '^\s*[+-]\s*(//|\*|/\*)' | head -20
```
Expected: empty, or only lines that are clearly inside block comments. **Any hit that is real code means the repair strayed out of comments — stop and investigate.**

- [ ] **Step 5: Build and test**

```
cmake --build build-msvc --target gowtoolkit_tests
ctest --test-dir build-msvc
```
Expected: 5/7, same two failures.

- [ ] **Step 6: Commit**

```bash
git add Source tests
git commit -m "style: Repair cp1252 mojibake in source comments"
```

---

### Task 3: Rewrite CLAUDE.md against the real tree

`CLAUDE.md`'s Layer Stack section is inverted — it describes as locally-owned everything that is now Onyx-owned, omits everything that is actually local, and states "There is no test suite" when a wired CTest suite with golden fixtures exists. It actively misleads anyone planning work here.

**Files:** Modify `CLAUDE.md`.

**Interfaces:** none.

- [ ] **Step 1: Correct the false claims**

Every one of these is wrong today and must change:

| Stale | True |
|---|---|
| `src/` (lowercase) throughout | `Source/` |
| `src/window/` — GLFW/ImGui lifecycle | Gone. `<Onyx/App/Window.h>`, Onyx-owned |
| `src/App.h/cpp` — main UI coordinator | Gone. `<Onyx/App/App.h>`. Local file is `Source/AppRegistration.{h,cpp}`, a thin registrar shim |
| `AssetDatabase`, `ProfileManager`, `IAssetProfile` as app types | All Onyx-owned; **and all three are removed in v1.1** |
| `src/core/schema/` — `StructDef` + `NodeInstance` | Gone. `Source/core/formats/` subclasses `<Onyx/Schema/AssetFormat.h>` |
| `src/core/vfs/` | Gone. `<Onyx/Vfs/...>` |
| `src/rendering/` — OpenGL `SceneRenderer` etc. | **Directory does not exist.** Onyx-owned, and Vulkan since v1.0.0 |
| `IAssetLoader` interface | Does not exist anywhere |
| "There is no test suite — testing is done manually" | **False.** See below |

- [ ] **Step 2: Document the test suite that exists**

Replace the "no test suite" line with the truth: `tests/` holds 20 sources on doctest + nlohmann_json (both fetched via `FetchContent`, gated on `GOWTOOLKIT_BUILD_TESTS`, default ON), wired into CTest as `unit`, `Golden_GOW2`, `Golden_GOWR`, `Metrics`, `Logger`, `Threading`, `ThemeContrast`. Fixtures are real truncated game WADs with pinned JSON snapshots (`tests/fixtures/README.md` documents provenance; `tools/make_test_fixtures.py` regenerates). State the current baseline honestly: 5/7, with `unit` and `ThemeContrast` failing on a theme-luminance invariant the engine changed.

- [ ] **Step 3: Document what IS local**

The file never mentions the app's actual content: `Source/core/parsers/gow2/` and `gowr/` (the format parsers), `Source/core/profiles/` (per-game container logic), `Source/core/formats/` (byte-layout schemas), `Source/core/types/handlers/`, `Source/core/shaders/DxilDisassembler`, and the surviving local UI (`WadBrowser`, `Inspector`, `MapViewer`, `MaterialViewer`, `SoundPlayer`, `RoleVisuals`, `CodeView`).

- [ ] **Step 4: Fix the build commands section**

It says `mkdir -p build && cd build && cmake -G Ninja ...`. Add that MSVC is required and that a bare configure may pick up a GCC on PATH and fail inside Onyx. Name the `vcvars64.bat` step.

- [ ] **Step 5: Flag, do not delete, the Go reference path**

Line 70 points at `/Users/jeanxpereira/CodingProjects/god_of_war_browser`, a macOS path unreachable from this Windows machine. It is still the authoritative GOW2 reference. Keep it, note it is machine-specific.

- [ ] **Step 6: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: Rewrite CLAUDE.md against the real tree"
```

---

### Task 4: Fix the build file's dead references and triage scratch/

**Files:** Modify `CMakeLists.txt`. Inspect `scratch/`.

- [ ] **Step 1: Correct the local-override comment**

`CMakeLists.txt:14` documents the local-dev override as
`-DFETCHCONTENT_SOURCE_DIR_ONYXSDK=C:/CodingProjects/Personal/OnyxSDK`. That path
does not exist; the real checkout is `E:/CodingProjects/OnyxSDK`. Correct it.

- [ ] **Step 2: Report on `scratch/`, do not delete**

`scratch/` holds `exp` (17,928 bytes, no extension), `experiment.cpp` (114 bytes),
`extract_gnf.py` (72 bytes). None is referenced by CMake or docs. **Do not delete
them** — report what each contains in your task report so the owner decides. A
72-byte extraction script may encode a format insight worth keeping.

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: Correct the stale local-override path"
```

---

### Task 5: Move the pin to Onyx v1.1

**Files:** Modify `CMakeLists.txt:15-17`.

- [ ] **Step 1: Repoint `GIT_TAG`**

```cmake
FetchContent_Declare(OnyxSDK
    GIT_REPOSITORY https://github.com/JeanxPereira/OnyxSDK.git
    # v1.1.0's tag does not exist yet -- PR #1 is open and the tag is cut at
    # merge. Pinned to the branch head until then; swap this for v1.1.0 once
    # the tag exists.
    GIT_TAG        dd064f8b1e4c3d9f6a2b8e7c5d4a3f1e9b0c8d7a)
```

**Use the real full SHA** — resolve it with `git -C E:/CodingProjects/OnyxSDK rev-parse dd064f8` rather than trusting the abbreviated form written here.

- [ ] **Step 2: Reconfigure and observe the new failures**

```
cmake -S . -B build-msvc -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-msvc --target gowtoolkit_tests
```

This will surface a wave of errors — that is expected and is the point. Capture the full error list to a file; it is the work-list for Tasks 6-8 and for later phases:

```
cmake --build build-msvc --target gowtoolkit_tests 2>&1 | grep -E "error" > /tmp/port-p1-errors.txt
wc -l /tmp/port-p1-errors.txt
```

Report the distinct error categories in your task report. Do not try to fix them all here.

- [ ] **Step 3: Commit the pin alone**

```bash
git add CMakeLists.txt
git commit -m "build: Pin Onyx to the v1.1 branch head"
```

---

### Task 6: Restore `AssetEntry` and `AssetContainer` to the app's namespace

245 call sites across 25 files referenced these as global names. v1.1 has them only under `Onyx::Domain::`.

**Files:** Modify `Source/core/WadTypes.h`.

**Interfaces:**
- Produces: global `AssetEntry` / `AssetContainer` names remaining valid for every existing call site.

- [ ] **Step 1: Add the aliases**

In `Source/core/WadTypes.h`, after the existing `<Onyx/Domain/Entry.h>` and `<Onyx/Domain/Wad.h>` includes:

```cpp
// Onyx v1.0.0 removed the global-scope aliases for these two (its audit gap G5).
// The app reaches them through this umbrella header in 245 places across 25
// files, so they are re-introduced here rather than qualified at every site --
// a mechanical sweep of that size would bury the port's real changes.
// Deleting WadTypes.h entirely is a separate, already-planned piece of work.
using AssetEntry     = Onyx::Domain::AssetEntry;
using AssetContainer = Onyx::Domain::AssetContainer;
```

- [ ] **Step 2: Verify the error count dropped**

```
cmake --build build-msvc --target gowtoolkit_tests 2>&1 | grep -E "error" > /tmp/port-p1-errors-2.txt
diff /tmp/port-p1-errors.txt /tmp/port-p1-errors-2.txt | head -20
wc -l /tmp/port-p1-errors-2.txt
```
Report the before/after counts. Expect a large drop; remaining errors belong to later tasks.

- [ ] **Step 3: Commit**

```bash
git add Source/core/WadTypes.h
git commit -m "refactor: Re-alias the Domain entry types for the app"
```

---

### Task 7: Sweep the logging macros

215 call sites across 25 files use `LOG_INFO` / `LOG_WARN` / `LOG_ERR` / `LOG_DEBUG`. v1.1 keeps those spellings only behind `ONYX_LEGACY_LOG_MACROS` — opt-in, never on by default, precisely so the SDK stops capturing a consumer's own `LOG_INFO`.

**Decision made for you:** do the rename, do NOT define the legacy escape hatch. The macro names are the SDK's; taking the escape hatch means the app permanently owns four of the most commonly `#define`d identifiers in C++.

**Files:** 25 files. Highest counts: `GOWRLoaders.cpp` (31), `AnimationParser.cpp` (18), `gowr/MeshParser.cpp` (18), `LodPackIndex.cpp` (18), `TextureParser.cpp` (16).

- [ ] **Step 1: Sweep**

```bash
grep -rlE '\bLOG_(INFO|WARN|ERR|DEBUG)\b' Source | while read -r f; do
  sed -i -E 's/\bLOG_(INFO|WARN|ERR|DEBUG)\b/ONYX_LOGF_\1/g' "$f"
done
```

**Check the target spelling before running this.** v1.1 has two families:
`ONYX_LOG_TRACE/DEBUG/INFO/WARN/ERROR(cat, fmt, ...)` takes a category argument,
while `ONYX_LOGF_DEBUG/INFO/WARN/ERR(fmt, ...)` is the printf-style one with no
category. The app's existing calls are printf-style with no category, so
`ONYX_LOGF_*` is the mechanical match — note `ERR` stays `ERR` in that family
(not `ERROR`). Verify against `Include/Onyx/Services/Logger.h` in the fetched
source before sweeping, and fix the sed if the spellings differ.

- [ ] **Step 2: Verify none survived**

```bash
grep -rnE '\bLOG_(INFO|WARN|ERR|DEBUG)\b' Source | wc -l
```
Expected: 0.

- [ ] **Step 3: Build and check the error delta**

```
cmake --build build-msvc --target gowtoolkit_tests 2>&1 | grep -E "error" | wc -l
```

- [ ] **Step 4: Commit**

```bash
git add Source
git commit -m "refactor: Move logging onto the ONYX_ macro family"
```

---

### Task 8: Move entry payload addressing onto `ByteRange`

`AssetEntry` addressed its payload with raw `uint32_t offset` / `size`. v1.1 uses `Domain::ByteRange source{fileIndex, offset, size}` with **64-bit** offset and size.

**Files (~40 sites across 12):** the write sides are `Source/core/profiles/gow2/ProfileGOW2.cpp:153,219,225,258,363` (builds entries inline) and `Source/core/profiles/gowr/WadNodeBuilder.cpp:667,689` (`ToNode`, `MakeFolder`). The read sides are every `SliceFile(...)` / `Seek(...)` construction: `GOWRLoaders.cpp` (13 sites), the gow2 parsers, the type handlers, `CliApp.cpp:84`, `Inspector.cpp:56`, `WadBrowser.cpp:213,264`.

**Interfaces:**
- Consumes: `Onyx::Domain::ByteRange`.
- Produces: entries whose payload is addressed via `entry.source.offset` / `entry.source.size`, with `fileIndex` defaulting to 0 (the container file itself).

- [ ] **Step 1: Read sides first**

`entry.offset` → `entry.source.offset`, `entry.size` → `entry.source.size`. Leave `fileIndex` at its default 0 — this phase does not introduce mounted-VFS indices; that arrives with the modules in Phase 2.

Be careful: several `.offset` hits in the tree are **not** `AssetEntry` and must not be touched — `LodEntry` (`LodPackIndex.cpp`), `GOWRFileDesc`/`RawEntry` (`ProfileGOWR.cpp`, `WadNodeBuilder.cpp`), animation manager offsets (`AnimationParser.cpp`), DXBC `chunk.offset` (`GOWRLoaders.cpp:1260`), and `MeshParser.cpp:783`'s `entry->offset` which is a `LodEntry*`. Check the declared type at each site.

- [ ] **Step 2: Write sides**

In `ProfileGOW2.cpp` and `WadNodeBuilder.cpp`, assignments like `e.offset = r.offset;` become `e.source.offset = r.offset;` with `e.source.size` alongside.

- [ ] **Step 3: Audit the narrowing**

`ByteRange::offset` and `size` are `uint64_t`; the app's consumers are `uint32_t`-shaped (`std::vector<uint8_t> buf(entry.size)`, `SliceFile(..., entry.size)`). These still compile while silently narrowing.

Report every site where a 64-bit value flows into a 32-bit parameter. **Do not fix them in this task** — for GOW2/GOWR WADs the values genuinely fit in 32 bits, and changing the signature of `SliceFile` calls is out of scope here. The deliverable is the list, so the risk is named rather than discovered later.

- [ ] **Step 4: Build and check the delta**

```
cmake --build build-msvc --target gowtoolkit_tests 2>&1 | grep -E "error" | wc -l
```

- [ ] **Step 5: Run the regression gate**

```
ctest --test-dir build-msvc
```

If `gowtoolkit_tests` links at this point, `Golden_GOW2` and `Golden_GOWR` **must** still pass — they parse real WADs and this task rewrites how entries address their bytes. A golden failure here means the offset migration is wrong, not that the golden is stale. If the target does not link yet because of errors belonging to later phases, say so plainly in your report rather than claiming the gate passed.

- [ ] **Step 6: Commit**

```bash
git add Source
git commit -m "refactor: Address entry payloads through ByteRange"
```

---

## Self-review notes

- **Spec coverage.** Decision 1 (`WadTypes.h` aliases) → Task 6. Decision 6 (cleanup first, own phase) → Tasks 1-4. The pin discussion → Task 5. `LOG_*` and `ByteRange` from the mechanical inventory → Tasks 7-8. Decisions 2-5 (modules, materials, CLI, the two stray GL surfaces) are Phases 2-5 and deliberately absent here.
- **Ordering is load-bearing twice.** BOMs before mojibake, because the BOM set is a superset and stripping first gives a clean marker. Double-encoded mojibake before single-encoded, because the former contains the latter's byte pattern and repairing outside-in leaves half-fixed files.
- **The app target is not expected to build in this plan.** Only `gowtoolkit_tests`. Any task claiming a green app build is claiming something impossible before Phase 3.
- **`DecodedText`** (the `Onyx::Modules::TextOut` rename) has no call site in the app today — the app never used that type. It appears in the spec's mechanical list for completeness and needs no task.
