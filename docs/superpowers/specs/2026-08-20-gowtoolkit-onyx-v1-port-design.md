# GoWToolkit → Onyx v1.1: port design

Status: DESIGN — awaiting approval
Predecessor state: pinned to OnyxSDK `v0.6.0`, on branch `feat/gowr-lod-rig-shaders`
Target: OnyxSDK v1.1 (`dd064f8`, PR JeanxPereira/OnyxSDK#1 — tag `v1.1.0` does not exist yet)

## The thing to understand before anything else

**The app does not currently compile against the version it claims to pin.**

`CMakeLists.txt:17` says `GIT_TAG v0.6.0`. The code calls `SceneData::pbrLayers`
(`GOWRLoaders.cpp:969`, `RenderCommand.cpp:176`, `AssetHarness.cpp:138`). That
field was *introduced* in `a591528` and *removed* in `0fe8ef3`, both between
v0.6.0 and v1.0.0 — it exists in **no published Onyx tag**. The recent GOWR work
was written against a local OnyxSDK checkout mid-flight, and the pin was never
moved to match.

So this is not "a working app that we are upgrading". It is an app that only ever
built against a moment in another repo's history that no longer exists. The port
is what makes it buildable again, not an optional modernization.

That reframes the risk: there is no green baseline to regress *from* on the app
target. There is one on the test target, and it is the only baseline we have.

## The baseline we do have, measured

`gowtoolkit_tests` builds and runs today (it links a subset that excludes the
broken `GOWRLoaders.cpp`; `test_stubs.cpp` stubs the loader symbol):

```
5/7 passing — Golden_GOW2, Golden_GOWR, Metrics, Logger, Threading
2/7 failing — unit, ThemeContrast
```

Both failures are one bug: `tests/theme_contrast_test.cpp:94` asserts dark-theme
surface luminance ≤ 0.35 and Onyx returns 0.38–0.55. The app pins colour
invariants the engine changed when it gained its Appearance module. This is
**pre-existing debt from the previous bump, not port damage**, and v1.1 may move
those colours again — so it is addressed at the END of the port, against the
final theme, not at the start against a moving one.

Golden_GOW2 and Golden_GOWR are the two that matter most: they parse real
truncated WADs (`R_BOAR00.WAD` PS2 USA, `r_athena00.wad` PC) against pinned JSON
snapshots. **They are the regression gate for the entire port.** If the module
rewrite changes what the parsers produce, these two go red — that is the whole
point of them.

## What must not be damaged

The reverse-engineering knowledge is the irreplaceable asset here; the Onyx
plumbing is replaceable by definition. Verified safe:

- **`Source/core/formats/` (4 files)** subclass `Onyx::Schema::AssetFormat` and
  encode byte layouts directly (`Struct("GOW2Instance", 76, Hex("padding", 0x00, 28), UInt16("id", 0x1C), ...)`).
  This was the port's biggest unknown. **Measured: `Schema/AssetFormat.h` is
  byte-identical between v0.6.0 and v1.1.** These files compile untouched.
- **`Source/core/parsers/gow2/` (10 parsers)** and **`Source/core/parsers/gowr/`
  (8+ parsers)** touch only leaf Onyx API — `Vfs::IFile`, `SliceFile`, `Logger`,
  `Parsers::*Data`, `Audio::AdpcmDecoder`. **None touches the profile layer.**
  They survive the port with a `LOG_*` rename and nothing else. The one to watch
  is `TexPackIndex.cpp`, the only file using `Services::TaskManager`.
- **`docs/GoW1/Formats/` + `docs/GoW2/Formats/` + `docs/GoWRknk/Formats/` (48
  documents)** — format knowledge, verified as genuinely distinct per generation
  (GoW1 vs GoW2 same-named files differ). Untouched by this work.

## Design decisions

### 1. `WadTypes.h` keeps the app compiling across 245 call sites

`AssetEntry` and `AssetContainer` moved from global scope into `Onyx::Domain::`.
That is 245 call sites across 25 files — nearly every file that parses or
displays a WAD.

**Decision: re-introduce the two names via `using` in `Source/core/WadTypes.h`,
the umbrella header they already flow through.** Two lines instead of 245 edits.

The app's own backlog wants `WadTypes.h` deleted eventually. That is a separate
decision and a separate diff; making it here would multiply this port's review
surface by an order of magnitude for zero functional gain, and would bury the
changes that actually matter under a mechanical rename.

### 2. The profile layer becomes two `IGameModule`s, and probing changes meaning

`ProfileGOW2` (417 lines), `ProfileGOWR` (234), `WadNodeBuilder` (748). All three
are pure binary-format parsers whose only engine surface is
`AssetEntry`/`AssetContainer`/`TypeId`/`Vfs::IFile` — they port mechanically once
those types resolve.

What does **not** port mechanically is detection. `IAssetProfile::Detect()`
returned a bool and the first match won. `IGameModule::Probe()` returns a
confidence 0-100, and `RankProbes` requires the winner to score ≥ 40 **and be
strictly greater than the runner-up** — an exact tie means nobody opens the file.

Today's logic is asymmetric in a way that hides this: `ProfileGOW2::Detect`
accepts any `.wad` *unless* it sniffs a GOWR magic (`WTOC`/LZ4), deferring by
exclusion. Ported naively, both modules would return the same confidence on a
plain `.wad` and every such file would fail to open with "ambiguous".

**Decision: score on evidence, not on extension.** GOWR scores high on a real
`WTOC`/LZ4 magic match and low otherwise; GOW2 scores moderate on extension alone
and higher when it sniffs a GOW2 tag stream. The existing "unless it looks like
GOWR" exclusion disappears — it was compensating for first-match-wins semantics
that no longer exist.

### 3. `pbrLayers` and `MaterialInfo` become `TextureRole`

The removed model was positional: `MaterialInfo::Layer[]` with a convention that
layer 0 is "main", plus a nested `SceneData::textures` as
`vector<vector<unique_ptr<TextureData>>>`, plus a `pbrLayers` bool telling the
renderer which convention to read.

v1.1 is explicit: `MaterialDesc::textures` is a `std::map<TextureRole, int>`
indexing a **flat** `SceneData::textures` pool. A role absent from the map means
"no map for that role" — not index 0.

**This is the change the app was already moving toward.** Commit `defbf05`
("Decode tangents and stage material textures by PBR role") staged textures by
role against a layered container. The port finishes that thought in the engine's
own vocabulary and deletes the `pbrLayers` flag, which existed only to
disambiguate two conventions that are now one.

The three consumers of the old shape — `AssetHarness::PrintSceneStats`,
`RenderCommand`'s JSON report, and the GOWR build path — move with it.

### 4. The CLI mostly gets deleted, not ported

`Source/cli/HeadlessGL.{h,cpp}` (227 lines of GLFW + hand-built MSAA FBO pair +
`glReadPixels`) and `RenderCommand.cpp`'s renderer driving both exist because
v0.6.0 had no headless render entry point. v1.1 has two: `Onyx::Rendering::RenderToImage`
and `Onyx::Cli::CmdRender`.

`Onyx::Cli::CmdRender`'s signature is `RenderFn` with defaults stripped, so it
drops into `Onyx::Cli::Run`'s hook with **zero adapter code**:

```cpp
Onyx::Modules::Workspace ws(Onyx::Types::TypeCatalog::Get());
ws.AddModule(std::make_unique<GowrModule>());
ws.AddModule(std::make_unique<Gow2Module>());
return Onyx::Cli::Run(ws, argc, argv, std::cout, std::cerr,
                      Onyx::Cli::MakeGltfExportFn(true, true),
                      Onyx::Cli::CmdRender);
```

**Decision: delete `HeadlessGL` entirely and reduce `RenderCommand` to the hook
above.** Deleting 227 lines of duplicated GL context management beats porting it
to Vulkan — the duplication was acknowledged in `HeadlessGL`'s own comment
("duplicates ~40 lines of `Viewport3D::ResizeFBO` because that method is
private").

One capability is **gained** for free: the CLI cannot open an ISO today
(`AssetHarness` rejects `.iso` outright because it does not reimplement
`AssetDatabase::LoadPakFromIso`). `Workspace` has `MountSpec` natively, so the
ISO path stops being GUI-only.

One capability is **lost and must be declared**: `RenderCommand`'s JSON report
emits raw GL texture names per batch (`b.texture0`, `b.texAO`, …), and its
headline number is "GL texture id 0 == no diffuse texture". Vulkan has no
equivalent stable small integer. The report's texture block becomes
role-and-boolean shaped (`{"role": "Diffuse", "bound": true}`), which answers the
same question the old field was actually being used to answer.

### 5. Two GL surfaces exist outside the renderer, and one is a real port

- **`Source/ui/viewers/MaterialViewer.cpp`** keeps its own `GLuint` texture cache
  and hands raw GL names to `ImGui::Image()`. Porting `SceneRenderer` does not
  cover this. v1.1's `Onyx::App::TexturePool::RegisterExternalView` is the
  intended replacement (Onyx's own `VideoPlayer` already uses it).
- **`GOWRLoaders.cpp:284`** — `GowrLodDocument::Apply()` reaches
  `Viewport3D::GetSceneRenderer()` and mutates `RenderBatch::isVisible` directly
  to drive the GOWR LOD picker. This is the one place the app depends on renderer
  internals. Note the timing: **Onyx v1.1 is the release that made the Vulkan
  renderer honor `isVisible`** — before it, this picker would have been inert
  even after a successful port.

### 6. Cleanup rides along, but as its own phase

46 files carry cp1252 mojibake, 6 of them doubly corrupted
(`Ã¢â‚¬â€` — corrupted, saved, corrupted again). 76 files carry a UTF-8 BOM, and
the BOM set is a strict superset of the mojibake set: same tool, one round-trip.
Zero mojibake in `docs/` — the Portuguese there is correctly encoded, and a naive
regex would have destroyed it.

`CLAUDE.md`'s entire Layer Stack section is inverted: everything it calls local
(`App`, `Window`, `AssetDatabase`, `ProfileManager`, `schema/`, `vfs/`,
`rendering/`) is Onyx-owned, and everything actually local (the profiles, the
parsers, `formats/`, the surviving viewers) goes unmentioned. It also states
"There is no test suite", which is flatly false and actively misleading to anyone
— human or agent — planning work here.

**Decision: cleanup goes FIRST, in its own phase, before any API change.** It is
verifiable against the one baseline we have (5/7 tests), it touches no semantics,
and doing it first means the port's diff is not buried under 46 files of
whitespace-invisible encoding churn.

## Phasing

Each phase ends somewhere defensible. Only Phase 1 has a green app build as its
gate, because there is no green app build to preserve until then.

| Phase | Delivers | Gate |
|---|---|---|
| **0 — Cleanup** | mojibake, BOMs, `CLAUDE.md` rewritten, dead CMake path, `scratch/` triage | tests still 5/7; zero semantic diff |
| **1 — Mechanical compat** | pin → v1.1 SHA, `LOG_*` sweep, `WadTypes.h` aliases, `ByteRange`, `DecodedText` | compiles further than before; tests still 5/7 |
| **2 — The modules** | `Gow2Module` + `GowrModule` as `IGameModule`; probe scoring; `Workspace` wiring | **Golden_GOW2 + Golden_GOWR green** — the port's real gate |
| **3 — Materials** | `TextureRole`/`MaterialDesc`; `pbrLayers` deleted | app target compiles |
| **4 — Shell** | composition root, `WadBrowser`, `MaterialViewer` on `TexturePool`, LOD picker | app runs; eye-pass |
| **5 — CLI** | delete `HeadlessGL`, `RenderCommand` → `Onyx::Cli::Run` | `probe`/`list`/`extract`/`decode`/`render` work; ISO opens headlessly |
| **6 — Close** | theme test re-tuned against final colours, docs, README | **7/7 green** |

## Out of scope, declared

- Deleting `WadTypes.h` (the app's own backlog item) — see decision 1.
- The `linux-lavapipe` CI failures in OnyxSDK — that is the engine's debt.
- Rewriting `docs/roadmap/` — stale, historical, harmless.
- `docs/superpowers/` from the 2026-06 SDK split — historical record, correctly
  left as-is.

## The pin, and why it is a SHA

`v1.1.0` does not exist as a tag. PR #1 is open and unmerged; the tag is created
at merge. The pin therefore reads `dd064f8` with a comment naming the tag it
should become. A SHA is honest and reproducible; waiting would block this work on
a GUI eye-pass in another repo.
