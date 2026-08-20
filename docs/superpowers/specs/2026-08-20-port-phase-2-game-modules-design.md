# Port Phase 2 — The game modules

Status: DESIGN — awaiting approval
Predecessor: Phase 0+1 complete (`a1c3a30..18f958b`, 11 commits). Pin is Onyx v1.1
(`3861911`). App target 113 errors, test target 43 — all of them this phase's and
the later phases' scope.
Parent spec: `docs/superpowers/specs/2026-08-20-gowtoolkit-onyx-v1-port-design.md`

## What this phase is

Onyx v1.1 deleted `IAssetProfile`, `ProfileManager` and `AssetDatabase`. In their
place a consumer implements `IGameModule` and registers it on a `Workspace`. This
phase converts `ProfileGOW2` (417 lines), `ProfileGOWR` (234) and the
`WadNodeBuilder` (748) onto that contract.

**The gate is `Golden_GOW2` and `Golden_GOWR`.** They parse real truncated game
WADs — `R_BOAR00.WAD` (PS2 USA) and `r_athena00.wad` (PC) — against pinned JSON
snapshots. Every other phase can argue about what "working" means; this one
cannot. If the ported modules produce a different tree than the profiles did, the
goldens go red, and that is the answer.

Those goldens have not run since the pin moved, because the test target does not
link. **Getting them green again is this phase's definition of done.**

## The interface being ported onto

```cpp
class IGameModule {
    virtual ModuleInfo  Info() const = 0;
    virtual ProbeResult Probe(const ProbeInput&) const = 0;
    virtual void        RegisterTypes(Types::TypeRegistrar&) = 0;
    virtual void        RegisterDecoders(DecoderRegistry&) = 0;
    virtual std::vector<MountSpec> Mounts() const { return {}; }
    virtual ParseResult ParseContainer(ContainerContext&) = 0;
};
```

`ParseContainer` receives a `ContainerContext` carrying the file, settings, a
`DiagSink`, a `Progress`, a `ModuleState` (`shared_ptr<void>`, per **document**),
the `roots` vector to fill, and — when a `MountSpec` matched — a `mountedVfs` and
a `fileTable` whose index 0 is pre-seeded with the container file.

## Decision 1: probing scores evidence, and the old logic inverts

`IAssetProfile::Detect()` returned bool, first match won. `IGameModule::Probe()`
returns 0-100, and `RankProbes` requires the winner to score **≥ 40 and strictly
above the runner-up** — an exact tie means no module opens the file at all.

Today's logic is built on first-match semantics in a way that breaks under
ranking: `ProfileGOW2::Detect` accepts *any* `.wad` **unless** it sniffs a GOWR
magic (`WTOC`/LZ4), deferring by exclusion. Ported literally, both modules would
return the same confidence for a plain `.wad` and every such file would fail to
open as ambiguous.

**Decision: score positive evidence, and delete the exclusion.**

| Module | Evidence | Score |
|---|---|---|
| GOWR | `WTOC` magic, or an LZ4 frame header | 95 |
| GOWR | `.wad` extension, no GOWR magic | 10 |
| GOW2 | a readable GOW2 tag stream in the header | 80 |
| GOW2 | `.iso` extension | 90 |
| GOW2 | `.wad` extension, nothing else | 45 |

The "unless it looks like GOWR" clause disappears — it was compensating for
first-match-wins, and under ranking the same outcome falls out of GOWR simply
scoring higher on real evidence. `Probe` gets 64 KiB of header and must stay pure.

## Decision 2: `profileTag` stops being stored, because it was always derived

This is the phase's one genuinely new problem. `AssetEntry::profileTag` — a
type-erased per-entry slot — **does not exist in v1.1**, and v1.1 offers no
per-entry storage at all (`ModuleState` is per *document*). Eight call sites
depend on it, including the GOWR role/taxonomy layer and two UI panels.

What it held:

```cpp
struct GowrProfileTag {
    WadEntryRole role;        // ~30 semantic roles
    WadBlock     block;
    WadAssetName parsedName;  // prefix, base, lod, variant, wadIndex
};
```

Measured, field by field:

- **`parsedName` was never stored data.** `WadNodeBuilder.cpp:672` builds it as
  `WadAssetName::Parse(r.name)` — a pure function of the entry's own name.
- **`block` is written and never read.** `e.block = currentBlock` at
  `WadNodeBuilder.cpp:69,95`; no consumer of the tag reads it. It is builder-local
  state that leaked into a persisted structure.
- **`role` is already encoded in `typeId`** — `e.typeId = RoleToTypeId(r.role)`
  (`:669`). The mapping is *almost* invertible: 26 cases, 25 distinct types.
  The only collapse is three texture roles onto one type:
  `TexturePair`, `TextureGpu` and `TextureCpu` all map to `GameTypes::TexturePair`.
  And that distinction is precisely the one the taxonomy documents as derived —
  `TextureGpu` is "`TX_*` with large size", `TextureCpu` is "`TX_*` with small
  size". Name and size, both on the entry.

**Decision: delete the tag and reclassify on demand.** A free function
`Gowr::Classify(const Domain::AssetEntry&) -> GowrProfileTag` reproduces the
original classification from `name`, `source.size` and `typeId`. Callers that
read the tag call it; nothing stores it.

Three reasons this beats inventing per-entry storage:

1. Nothing is lost. Every field is either pure-derived already or dead.
2. A side-table keyed on `NodePath` would have to be rebuilt whenever the builder
   reorders or regroups nodes — which it does extensively, synthesising folders
   for shader stages, LOD and particle groups.
3. The surviving axis, `typeId`, is Onyx's own. `TypeCatalog` already carries a
   colour and an icon per type — which is what `Source/ui/RoleVisuals.h` currently
   hand-rolls. That convergence is a later cleanup, not this phase, but the
   direction is worth not walking away from.

**The risk, stated:** `Classify` must reproduce the original classifier exactly,
and the goldens only cover what the two fixture WADs contain. A role that appears
in neither fixture is unverified. That is a real gap and it goes in the phase's
Known-gaps note rather than being papered over.

## Decision 3: the ISO path becomes a `MountSpec`, and the CLI gains ISO for free

`ProfileGOW2` mounts ISOs through `MountArchive` + `LoadFromArchive`, and the
headless harness explicitly refuses `.iso` ("ISO input is not supported headlessly
yet") because it does not reimplement `AssetDatabase::LoadPakFromIso`'s PAK-slice
walk. Only the GUI and `CliApp::HandleExtract` — which bypasses the harness
entirely — can open one.

v1.1 has `MountSpec` natively: a module declares the extensions it treats as an
inner VFS plus a factory, and `ParseContainer` then receives `mountedVfs` and a
`fileTable`. The GOW2 module declares `.iso`, and entries inside reference their
file through `ByteRange::fileIndex` instead of the flat single-file assumption.

**This closes a real capability gap rather than porting one across.** It is also
the first live use of `fileIndex` — Phase 0+1 deliberately left it at 0
everywhere, and this is where non-zero values start appearing.

## Decision 4: `ModuleState` carries what the profiles kept in members

`ProfileGOWR::ParseContainer` LZ4-decompresses the whole container into a
`MemoryFile` and stores it as `outWad.fileSource`, so every downstream `SliceFile`
reads the decompressed buffer rather than the original file. It also kicks off an
eager, detached-thread texpack index build.

Both become `ModuleState`: the decompressed file and the texpack index live in a
module-owned struct that decoders downcast to. The detached `std::thread` should
move onto the `Workspace`'s `JobQueue` — it already provides lane serialization
and cooperative cancel, which a detached thread does not.

`PrepareForParse` has no equivalent in `IGameModule` and does not need one: its
job (writing `config.ini` next to the WAD before the LOD index is built) happens
at the top of `ParseContainer`, which is the only ordering that ever mattered.

## Out of scope for this phase

Materials/`TextureRole` (Phase 3), the Shell composition root and the two stray GL
surfaces (Phase 4), the CLI (Phase 5), and the theme test (Phase 6). Also:
`Onyx::Api::Database()`/`GetSelected()` are gone and `WadBrowser` depends on them —
that is Phase 4's problem, and `WadBrowser` is expected to stay broken until then.

`tests/logger_test.cpp` tests a `GOW_LOG_*` family that never existed in this
repo. It is an orphan predating the port. Decision deferred to Phase 6.

## Definition of done

`Golden_GOW2` and `Golden_GOWR` green, from a build that actually links, with no
golden regenerated. Nothing weaker counts — and if the test target still cannot
link because of Phase 3+ errors, this phase is not done, however good the module
code looks.
