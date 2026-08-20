# Port Phase 2 — Game Modules Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert `ProfileGOW2`, `ProfileGOWR` and their tree builders onto Onyx v1.1's `IGameModule` contract, and get `Golden_GOW2` and `Golden_GOWR` green again from a build that actually links.

**Architecture:** The two profiles are pure binary-format parsers whose only engine surface is `AssetEntry`/`AssetContainer`/`TypeId`/`Vfs::IFile` — the parsing bodies port nearly verbatim. What changes is the frame around them: bool detection becomes evidence-ranked probing, `outWad` becomes `ctx.roots`, profile members become `ModuleState`, the ISO path becomes a `MountSpec`, and the per-entry `profileTag` stops being stored because every field it held was either derived or dead.

**Tech Stack:** C++20, CMake + Ninja, **MSVC via vcvars64**, doctest, CTest.

**Spec:** `docs/superpowers/specs/2026-08-20-port-phase-2-game-modules-design.md`

## Global Constraints

- **Build with MSVC.** A bare `cmake` resolves to Strawberry Perl's GCC and fails inside Onyx. Import `vcvars64.bat` first; build dir is `build-msvc/`. `build/` is a stale GCC configure — ignore it.
- **`Golden_GOW2` and `Golden_GOWR` are this phase's gate.** They parse real truncated WADs (`R_BOAR00.WAD` PS2 USA; `r_athena00.wad` PC) against pinned JSON snapshots. **Never regenerate a golden.** A golden failure means the port changed parser output — which is the one thing this phase must not do.
- **`ctest` runs whatever binary is on disk, even when the build failed.** Check the executable's timestamp against your edits before believing a green result. A previous task in this port was nearly fooled by exactly this.
- Entry payloads are already on `ByteRange` (Phase 1). `fileIndex` is 0 everywhere; this phase introduces the first non-zero values, via mounts.
- `AssetEntry`/`AssetContainer` reach the app as global aliases from `Source/core/WadTypes.h`. Leave that alone.
- Logging is `ONYX_LOGF_*` (printf-style, no category). Do not introduce `ONYX_LOG_*` (category family) or bare `LOG_*`.
- Commits: Conventional Commits, sentence-case subject, no trailing period. NEVER add any AI/Claude attribution or co-authorship trailer. Stage explicit paths — never `git add -A`.
- `sed -i` under Git-Bash silently converts CRLF→LF in this repo. If you use it, run `unix2dos` on touched files and check `git diff --numstat` for whole-file churn.

---

### Task 1: `Gowr::Classify` — recover the tag without storing it

`AssetEntry::profileTag` does not exist in v1.1 and v1.1 has no per-entry storage. Eight call sites read the tag. This task makes it derivable so the rest of the phase has somewhere to stand.

**Files:**
- Modify: `Source/core/profiles/gowr/GowrTaxonomy.h` / `.cpp` (declare and implement `Classify`)
- Test: `tests/gowr_classify_test.cpp` (create)
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `Onyx::Gowr::GowrProfileTag Classify(const Onyx::Domain::AssetEntry&);` — reconstructs role, parsed name, and a default block from the entry alone.

**Background — why each field is recoverable (verified, do not re-derive):**
- `parsedName` was never stored: `WadNodeBuilder.cpp:672` computes it as `WadAssetName::Parse(r.name)`, a pure function of the name.
- `block` is written at `WadNodeBuilder.cpp:69,95` and **read by nobody**. Leave it defaulted; if you find a reader, stop and report.
- `role` is encoded in `typeId` via `RoleToTypeId` (`WadNodeBuilder.cpp:631-662`) — 26 cases onto 25 types. The one collapse is `TexturePair`/`TextureGpu`/`TextureCpu` all mapping to `GameTypes::TexturePair`, and that split is documented in the taxonomy as size-based: `TextureGpu` is "`TX_*` with large size", `TextureCpu` is "`TX_*` with small size". Recover it from `entry.source.size` using the same threshold the original classifier used — **find that threshold in the existing code, do not invent one.**

- [ ] **Step 1: Write the failing test**

Create `tests/gowr_classify_test.cpp`. Build `Domain::AssetEntry` values by hand — name, `source.size`, `typeId` — and assert `Classify` returns the expected role and parsed name. Cover, at minimum: a vertex shader (`HASH_vs_FLAGS`), a pixel shader, a `MG_*_gpu` mesh, an `ANM_*` clip, a `MAT_HASH` with size > 0 and one with size == 0 (Material vs MaterialRef — a size-based split that already exists), a `TX_*` above and below the GPU/CPU threshold, and an unrecognised name falling to `Unknown`.

Name the cases with a `GowrClassify:` prefix and register a ctest entry filtering on it, matching how the existing suite is wired.

- [ ] **Step 2: Run it and watch it fail**

```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" && set" | ForEach-Object { if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "env:$($matches[1])" -Value $matches[2] } }
cmake --build build-msvc --target gowtoolkit_tests
```
Expected: `Classify` is not declared. If the target fails earlier on unrelated Phase 3+ errors, say so and work from a narrower TU.

- [ ] **Step 3: Implement `Classify`**

Reproduce the original classifier's decisions. **Read the code that currently assigns `r.role` before writing this** — it lives in the GOWR parse path and keys off name patterns and sizes. Your job is fidelity to it, not a fresh design.

- [ ] **Step 4: Watch it pass, then commit**

```bash
git add Source/core/profiles/gowr/GowrTaxonomy.h Source/core/profiles/gowr/GowrTaxonomy.cpp tests/gowr_classify_test.cpp tests/CMakeLists.txt
git commit -m "feat(gowr): Derive the entry tag instead of storing it"
```

- [ ] **Step 5: Repoint the eight readers**

`GOWRLoaders.cpp:39`, `WadNodeBuilder.cpp:16,442,445`, `Inspector.cpp:30`, `WadBrowser.cpp:23` read `e.profileTag.As<GowrProfileTag>()`. Replace each with a `Classify(e)` call. `WadNodeBuilder.cpp:445` *writes* a modified tag back — that write disappears; determine what it was for and report whether the behaviour it produced is preserved by reclassification. **If it is not, stop and report rather than guessing.**

Delete the two `e.profileTag = ProfileTag::Of(...)` writes at `:672` and `:690`.

```bash
git commit -m "refactor(gowr): Read the entry tag through Classify"
```

---

### Task 2: `GowrModule` — `IGameModule` for Ragnarök

**Files:**
- Create: `Source/core/modules/GowrModule.h` / `.cpp`
- Modify: `Source/core/profiles/gowr/ProfileGOWR.cpp` (body moves; file may be deleted at the end of this task)

**Interfaces:**
- Produces: `Onyx::Gowr::GowrModule : Onyx::Modules::IGameModule`, and a `GowrModuleState` the decoders downcast to.

- [ ] **Step 1: `Info()` and `Probe()`**

`Info()`: id `"gowr"`, display name, hints (`gowr`, `ragnarok`), and an `.wad` open filter.

`Probe()` scores evidence, per the spec — **95** for a real `WTOC` magic or LZ4 frame header in the supplied header bytes, **10** for a bare `.wad` with neither. `ProbeInput` gives you `path`, 64 KiB of `header`, and `fileSize`. Keep it pure: no file I/O beyond what you are handed. Return a `reason` string that names the evidence ("WTOC magic at 0", "LZ4 frame header") — it surfaces in the CLI's `probe` output and is what a user reads when detection surprises them.

- [ ] **Step 2: `RegisterTypes()`**

Mint the GOWR types through `TypeRegistrar::Add({key, label, media, icon, color})`. **Keys are bare** — `"mesh"`, not `"gowr.mesh"`; the registrar prefixes with the module id, and a key containing `.` is rejected. The app currently has its own `GameTypes` catalog; map each type it defines for GOWR onto a registrar call, and report any that do not survive the move.

- [ ] **Step 3: `ParseContainer()`**

Port `ProfileGOWR::ParseContainer` nearly verbatim. The changes:
- `outWad.entries` → `ctx.roots`
- The LZ4 decompression result — currently stored as `outWad.fileSource` so downstream `SliceFile`s read the decompressed buffer — goes into `GowrModuleState`, held by `ctx.state`.
- `PrepareForParse`'s work (`EnsureGowrConfigIni`, `InvalidateLodIndex`) moves to the **top** of `ParseContainer`. Its ordering requirement is real: skip it and the LOD index is built without `config.ini`, and the CLI parses a different mesh set than the GUI. That was the bug `AssetHarness.h`'s comment warned about.
- The eager texpack index build currently runs on a detached `std::thread`. Move it onto `ctx`'s job queue if reachable from the module, or keep it detached and **report that you did** — a detached thread outliving the document is a real hazard and I want it named either way.
- Report problems through `ctx.diags` rather than only logging. A malformed entry becomes a `Failed`-flagged node that stays in the tree (salvage), not a dropped one.

- [ ] **Step 4: `RegisterDecoders()`**

Wire the scene/image/text decoders the app already has for GOWR onto their `TypeId`s. Signatures: `SceneDecoder` returns `unique_ptr<Parsers::SceneData>`, `ImageDecoder` returns `unique_ptr<Parsers::TextureData>`, `TextDecoder` returns `optional<DecodedText>`. All take `DecodeContext&` (which carries `doc`, `entry`, `diags`, `progress`). Return null/empty on failure and report into `diags` — never throw.

**Materials are Phase 3.** If a decoder needs the `TextureRole` model, stub it to return null with a diag saying so, and list every stub in your report.

- [ ] **Step 5: Build and commit**

```bash
git commit -m "feat(gowr): Implement the Ragnarok game module"
```

---

### Task 3: `Gow2Module` — `IGameModule` for GOW1/GOW2, with the ISO mount

**Files:**
- Create: `Source/core/modules/Gow2Module.h` / `.cpp`
- Modify: `Source/core/profiles/gow2/ProfileGOW2.cpp`

**Note:** this is the file whose TU has **never compiled** in this port — a pre-existing fatal `#include <Onyx/Domain/IAssetProfile.h>` has masked it since the pin moved. Everything Phase 1 changed inside it is inspection-verified only. **This task is where the compiler finally sees it**, so expect errors that are not yours; triage them into "Phase 1 got this wrong" versus "this is the port" and report the split.

- [ ] **Step 1: `Info()` and `Probe()`**

Id `"gow2"`, hints (`gow2`, `gow1`), filters for `.iso` and `.wad`. Scoring per the spec: **90** for `.iso`, **80** for a readable GOW2 tag stream in the header, **45** for a bare `.wad`. **Delete the "unless it sniffs GOWR magic" exclusion** — under ranking, GOWR scoring 95 on real evidence beats GOW2's 45 without GOW2 having to know GOWR exists.

- [ ] **Step 2: `Mounts()` — the ISO becomes a real mount**

Declare a `MountSpec` for `.iso` whose factory returns a `Vfs::IsoFileSystem` (nullptr if it will not mount). `ParseContainer` then receives `ctx.mountedVfs` and `ctx.fileTable` with index 0 pre-seeded as the container file.

This is the first live use of `ByteRange::fileIndex` in the app — Phase 1 left it 0 everywhere. Entries that live inside a mounted file must carry the right index, and decoders resolve bytes through `ctx.doc.fileTable[fileIndex]`. Get this right and the CLI gains ISO support it never had; get it wrong and entries read from the wrong file.

- [ ] **Step 3: `ParseContainer()`**

Port the tag-stream walker verbatim — the 32-byte `RawWadTag` records, the `GROUP_START`/`GROUP_END` nesting stack, the `SERVER_INSTANCE` handling, and **both passes of zero-size reference resolution** (the `nameToDefinition` map plus the recursive `resolveUnknowns` catch for referenced-before-defined). That two-pass behaviour is load-bearing and subtle; preserve it exactly.

`LoadFromArchiveGOW2`'s TOC walk (the dual-layer-DVD sector math, `pakIndex = offset/10_000_000`) becomes the mounted path.

- [ ] **Step 4: `RegisterTypes()` / `RegisterDecoders()`** — as Task 2, for GOW2's types.

- [ ] **Step 5: Build and commit**

```bash
git commit -m "feat(gow2): Implement the God of War 1/2 game module"
```

---

### Task 4: Wire the modules and turn the goldens green

**Files:**
- Modify: `Source/main.cpp`, `Source/AppRegistration.cpp` (module registration)
- Modify: `Source/core/harness/AssetHarness.cpp` / `.h`
- Modify: `tests/golden_helpers.cpp`, `tests/CMakeLists.txt`
- Delete: the `IAssetProfile`-era files once nothing references them

**Interfaces:**
- Consumes: `GowrModule`, `Gow2Module`.
- Produces: a `Workspace` with both modules registered, reachable from tests and from the app.

- [ ] **Step 1: Registration**

`ProfileManager::Get().RegisterProfile(...)` becomes `Workspace::AddModule(std::make_unique<...>())`. In the GUI that runs inside `App::SetRegistrar`, before init. Note `AppRegistration.cpp` appears to hold a second, unused registration path with possibly-dead includes — determine which is live and report.

- [ ] **Step 2: The harness moves onto `Workspace`**

`AssetHarness::LoadContainer` currently selects a profile by hint or `DetectProfileForFile`, then calls `PrepareForParse` and `ParseContainer`. It becomes `Workspace::Open(path, moduleHint)` — probing, mounting and parsing all happen inside. Its `.iso` refusal can go: mounts handle it now.

- [ ] **Step 3: The goldens**

`tests/golden_helpers.cpp` walks a parsed tree and snapshots it to JSON. It must now walk `Document::roots` from a `Workspace`. **The snapshot's shape and field order must not change** — the pinned JSON files are the contract, and reformatting them is indistinguishable from breaking them.

- [ ] **Step 4: Run the gate**

```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" && set" | ForEach-Object { if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "env:$($matches[1])" -Value $matches[2] } }
cmake --build build-msvc --target gowtoolkit_tests
ctest --test-dir build-msvc -R "Golden" -V
```

**Confirm the test binary's timestamp is newer than your edits before believing any result.**

If a golden fails, diff the produced JSON against the pinned one and report the delta. A difference is a defect in the port — the fixtures are real game data and the snapshots were correct before the pin moved. **Do not regenerate.**

- [ ] **Step 5: Delete what is now dead**

`ProfileGOW2.{h,cpp}`, `ProfileGOWR.{h,cpp}` and their `IAssetProfile` includes, once nothing references them. `WadNodeBuilder` **survives** — it is GOWR's tree assembler and the module calls it.

- [ ] **Step 6: Commit**

```bash
git commit -m "feat(port): Open containers through the Workspace"
```

---

## Self-review notes

- **Spec coverage.** Decision 1 (evidence probing) → T2 S1, T3 S1. Decision 2 (`Classify`) → T1. Decision 3 (ISO mount) → T3 S2. Decision 4 (`ModuleState`) → T2 S3. Definition of done → T4 S4.
- **Ordering is load-bearing.** T1 first because six files read the tag and nothing else compiles until they have somewhere to read it from. T3 after T2 because `ProfileGOW2.cpp`'s TU has never compiled — meeting that surprise after the GOWR module is proven keeps the two failure sources separable.
- **The known gap this phase creates:** `Classify` is verified only against roles present in the two fixture WADs. Roles appearing in neither are unverified by construction. This goes in the phase's closing note.
- **`WadBrowser` stays broken through this phase** — `Onyx::Api` lost `Database()` and `GetSelected()`, and that is Phase 4. Do not fix it here; do not count it as a failure here either.
