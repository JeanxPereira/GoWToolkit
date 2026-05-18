> **Audience:** Claude Opus implementing this feature autonomously.  
> **Scope:** Replace the ad-hoc `getBaseGroupName` lambda in `ProfileGOWR.cpp` with a
> robust, multi-pass `WadNodeBuilder` that produces a semantically correct, hierarchical
> `ParsedEntry` tree for every Ragnarök WAD.  
> **Constraint:** All changes are additive or internal to GOWR. No other game profile,
> no existing interface (`IGameProfile`, `IAssetLoader`, `WadBrowser`, etc.) must be
> broken. The UI layer (`WadBrowser.cpp`) must not be changed — it already renders
> `ParsedEntry` trees recursively.

---

## Table of Contents

1. [Background — What We Know From Reverse Engineering](#1-background)
2. [Current Codebase State](#2-current-codebase-state)
3. [Target Architecture](#3-target-architecture)
4. [Data Model Changes — `WadTypes.h`](#4-data-model-changes)
5. [New File: `WadNodeBuilder.h`](#5-wadnodebuilder-h)
6. [New File: `WadNodeBuilder.cpp` — The Four Passes](#6-wadnodebuildercpp)
7. [Modifying `ProfileGOWR.cpp`](#7-modifying-profilegowrcpp)
8. [Updating `UIHelpers.h`](#8-updating-uihelpersh)
9. [File Locations Summary](#9-file-locations-summary)
10. [What NOT To Change](#10-what-not-to-change)
11. [Reference: Confirmed Binary Facts](#11-reference-confirmed-binary-facts)

---

## 1. Background

### 1.1 WAD File Format

God of War Ragnarök WAD files (`.wad`) are LZ4-framed containers with a `WTOC` v2
header followed by a flat array of `FileDesc` entries. Each entry is **144 bytes
(0x90)** with this confirmed layout:

```
Offset  Size  Field
+0x00   u32   size          — byte size of this asset's data (0 = no inline data)
+0x04   u32   offset        — resolved absolute offset into the decompressed file
+0x08   u32   offset2       — secondary offset (used in offset resolution algorithm)
+0x0C   u32   unk0C
+0x10   u32   unk10
+0x14   u16   unk14         — read by wadContext::GetTypeId (vtable slot +0x40)
+0x18   u32   unk18
...
+0x34   u16   group         — subsystem group identifier
+0x36   u16   type          — asset type enum (see §11.2)
+0x38   u8[8] unk38         — pattern: 0xBA/0xAA fill (hash/checksum area)
+0x44   u8    blockBitSet   — streaming block group; drives offset resolution
+0x45   u8[0x1F] unk2       — unk2[20] != 0 → also enqueue into group 8
+0x48   char[0x38] name     — null-terminated ASCII name (56 bytes max)
        ...remainder...
```

The `WTOC` header (16 bytes):
```
+0x00  u32  magic      = 0x434F5457  (\"WTOC\")
+0x04  u32  version    = 0x2
+0x08  u32  fileCount
+0x0C  u32  unk
```

Entries begin at offset `0x10` from the start of the decompressed data.

### 1.2 Functional Blocks Inside Every WAD

Empirical analysis of `r_fox00.wad` (286 entries) reveals that entries group into
**four functional blocks** separated by sentinel entries:

```
[Block: Manifest]    entries 0–29
  WAD_R_Fox00         — WAD identity / header
  TXRX_R_Fox00        — SharedWadRef → \"TXRX_Shared_Fox00\"
  WPTX_R_Fox00        — SharedWadRef → waypoints
  DLGX_R_Fox00
  ANMX_R_Fox00        — SharedWadRef → \"ANMX_Shared_Fox00\"
  LGTX_R_Fox00        — SharedWadRef → lighting
  MDLX_R_Fox00        — SharedWadRef → models
  MATX_R_Fox00        — SharedWadRef → \"MATX_Shared_Fox00\"
  PRTX_R_Fox00        — SharedWadRef → particles
  ... (more *X_R_ entries)
  PopHeap             — SENTINEL: marks end of Manifest block

[Block: Shaders]     entries 31–122
  0xHASH entries      — shader container descriptors
  depth_vs_*          — vertex shaders, 12 flag variants
  depvl_vs_*          — depth velocity vertex shaders
  opaque_vs_*         — opaque vertex shaders
  transp_vs_*         — transparent vertex shaders
  HASH_ps_FLAGS       — pixel shaders keyed by material hash
  autopad             — alignment padding between shader groups

[Block: Assets]      entries 123–235
  ANM_fox00           — animation clip
  TX_name_slot_HASH   — texture pairs (GPU sz=8448 + CPU sz=200)
  MAT_HASH            — material descriptors
  N_0_M entries       — LOD binding table (material→mesh index)
  MG_fox00_0_gpu      — mesh GPU buffer (needs lodpack)
  MESH_fox00_0        — mesh definition
  MDL_fox00           — model descriptor
  DCClientGUID        — client registration GUID (prelude to game objects)
  goProtofox00        — game object prototype
  gofox00             — game object instance
  gofox00_overrideInst— override instance
  SEMW_SNDFox00       — Wwise sound emitter (WwiseWadContext subsystem)
  autopad             — padding

[Block: Particles]   entries 236–285
  [Repeated 2×: one group per FX context (envRaceIntro/dust_landing, envRaceIntro/dust_takeoff)]
  For each group:
    DCClientGUID          — registration prelude
    PEM_emit_dust_*       — particle emitter definition
    DCClientGUID
    PEM_emit_bits_*
    DCClientGUID
    PEM_emit_debris_*
    DCClientGUID
    PTC_part_dust_*       — particle system
    MAT_HASH (sz=0)       — material reference (zero-size: pointer only)
    DCClientGUID
    PTC_part_bits_*
    MAT_HASH (sz=0)
    DCClientGUID
    PTC_part_debris_*
    MAT_HASH (sz=0)
    goProto*_context_name — FX game object prototype
    PEM_emit_dust_*       — secondary emitter (same context)
    go*_context_name      — FX game object instance
```

### 1.3 Key Naming Conventions

| Pattern | Examples | Meaning |
|---|---|---|
| `WAD_R_*` | `WAD_R_Fox00` | WAD identity entry |
| `*X_R_*` | `TXRX_R_Fox00`, `ANMX_R_Fox00` | SharedWadRef — resolves to `*X_Shared_*` |
| `PopHeap` | — | Sentinel: end of Manifest block |
| `autopad` | — | Alignment padding; no semantic content |
| `TX_name_slot_HASH` | `TX_angrboda_fox00_head_gen_0d_1D29...` | Texture. Slot suffix: `d`=diffuse, `n`=normal, `g`=gloss, `ao`=ambient occlusion, `p`=parallax |
| `MAT_HASH` | `MAT_04BAD27CE70EE0D6` | Material, keyed by shader hash |
| `MG_name_lod_gpu` | `MG_fox00_0_gpu` | Mesh GPU buffer for LOD 0 |
| `MESH_name_lod` | `MESH_fox00_0` | Mesh definition for LOD 0 |
| `MDL_name` | `MDL_fox00` | Model descriptor |
| `ANM_name` | `ANM_fox00` | Animation clip |
| `goProto*` | `goProtofox00` | Game object prototype |
| `go*` (no suffix) | `gofox00` | Game object instance |
| `go*_overrideInst` | `gofox00_overrideInst` | Instance override |
| `SEMW_*` | `SEMW_SNDFox00` | Wwise sound emitter (audio subsystem) |
| `PEM_emit_*` | `PEM_emit_dust_burst_thick` | Particle emitter |
| `PTC_part_*` | `PTC_part_dust_burst_thick` | Particle system |
| `DCClientGUID` | — | Registration prelude for the next non-zero-size entry |
| `HASH_vs_FLAGS` | `04bad27ce70ee0d6_vs_41e17653` | Vertex shader (hash is material hash, FLAGS is variant bitmask) |
| `HASH_ps_FLAGS` | `04bad27ce70ee0d6_ps_41e17653` | Pixel shader |
| `name_vs_FLAGS` | `depth_vs_00000216` | Named vertex shader |
| `0xHASH` | `0xC60E3D856F86AB98` | Shader container/descriptor |
| `N_0_M` | `0_0_1`, `3_0_21` | LOD binding table slot |

### 1.4 SharedWadRef Resolution (Ghidra Confirmed)

`FUN_14052f270` in `GoWR.exe` shows that `ANMX_R_Fox00` and `MATX_R_Fox00`
entries inside a WAD are resolved by constructing `\"ANMX_Shared_%s\"` and
`\"MATX_Shared_%s\"` from the WAD's base name. This is **not** a sub-WAD
hierarchy — it is a lateral reference to a separately-loaded shared WAD file.

Derivation rule: given entry name `ANMX_R_Fox00`:
- Strip the `*X_R_` pattern → base = `Fox00`
- Construct shared name = `ANMX_Shared_Fox00`
- The shared WAD file would be `ANMX_Shared_Fox00.wad` (or similar) on disk

The `*X_` prefix encodes the subsystem:
- `ANMX` → animations
- `MATX` → materials
- `TXRX` → textures
- `MDLX` → models
- `LGTX` → lighting
- `PRTX` → particles
- `COLX` → collision
- `ENTX` → entities
- `LSCX` → level scripting
- `BHVX` → behavior
- `CAMX` → camera
- `WWIX` → Wwise audio index
- `SCRX` → script
- `WPTX` → waypoints
- `GPSX` → GPS/nav
- `DCLX` → data components
- `INTX` → interaction
- `PHYX` → physics
- `WNDX` → wind
- `PCAX` → PCA (blend shape?)
- `CMRX` → camera rig
- `FX` → effects
- `GMO` → game object
- `CXT` → context
- `DLGX` → dialog
- `BTRX` → behavior tree
- `WWIX` → Wwise index

### 1.5 DCClientGUID Pairing (Ghidra Confirmed)

`wadContext::Enqueue` (vtable +0x28, `FUN_140550090`) inserts an entry into the
context's linked list and toggles `entry[4] &= ~1` (marks as \"loaded\"). The
`DCClientGUID` entry is enqueued first to register a client GUID, and the
subsequent asset entry follows. They are always adjacent in the flat list:

```
DCClientGUID   sz=37   → allocates GUID
PEM_emit_*     sz=381  → the actual asset
```

The `DCClientGUID` carries **no displayable content** — it is internal plumbing.
In the node tree it should be hidden (filtered from display) or folded into its
sibling.

### 1.6 Texture Pairs

Every logical texture appears **twice** in the flat entry list:
- Entry A: `TX_name_HASH` with `sz=8448` (or similar GPU size) — the compressed
  GPU texture data, to be resolved via lodpack
- Entry B: `TX_name_HASH` with `sz=200` (or similar small CPU size) — the
  descriptor/header used by the CPU

Both share the **exact same name**. They are distinguished only by size. In the
node tree they should appear as **a single node** with both sizes noted, or as a
parent node with two children labeled `[GPU]` and `[CPU]`.

### 1.7 WAD Category Flags (Ghidra Confirmed)

`FUN_140545820` matches the WAD name against this prefix table to assign a
category index:

| Index | Prefix | Length | Flag | Meaning |
|---|---|---|---|---|
| 0 | `WAD_R_W_` | 8 | `1` | World / level WAD |
| 1 | `WAD_R_I_` | 8 | `1` | Interactive objects |
| 2 | `WAD_R_` | 6 | `0` | Generic Ragnarök WAD |
| 3 | `WAD_Char_` | 9 | `0` | Character WAD |
| 4 | `WAD_Add_` | 8 | `0` | Additive / DLC WAD |
| 5 | `WAD_` | 4 | `0` | Fallback |
| 6 | *(none)* | 0 | `0` | Unknown |

Additionally, the constructor (`FUN_140546aa0`) gives special flag `0x10`
(high-priority streaming) to: `WAD_R_HeroA00`, `WAD_R_Atreus00`,
`WAD_R_FreyaValkyrie00`, `WAD_R_UI`, `WAD_Rbr_HomeBase200_SindriHouseInt`.

---

## 2. Current Codebase State

### 2.1 Relevant Files

```
src/
  UIHelpers.h                           — ColorForType(), IconForType() (needs updating)
  core/
    WadTypes.h                          — ParsedEntry, OpenWad, WadAssetName (needs new fields)
    WadAssetName.cpp                    — WadAssetName::Parse(), CanonicalName() (exists, unused)
    AssetDatabase.h / .cpp              — DO NOT CHANGE
    interfaces/IGameProfile.h           — DO NOT CHANGE
    loaders/IAssetLoader.h              — DO NOT CHANGE
    loaders/GOWRLoaders.h / .cpp        — DO NOT CHANGE
    profiles/gowr/
      ProfileGOWR.h                     — DO NOT CHANGE (signature)
      ProfileGOWR.cpp                   — ParseWad() needs to call WadNodeBuilder
  ui/
    WadBrowser.h / .cpp                 — DO NOT CHANGE
```

### 2.2 What `ProfileGOWR::ParseWad` Currently Does (and What's Wrong)

The current `ParseWad` in `ProfileGOWR.cpp`:
1. Decompresses LZ4 if needed ✅
2. Reads the WTOC header ✅
3. Reads all `GOWRFileDesc` structs ✅
4. Runs the blockBitSet offset resolution algorithm ✅
5. **Calls a lambda `getBaseGroupName(name)`** — this is the problem:
   - It strips known prefixes (`MDL_`, `MG_`, `MAT_`, etc.) from names
   - Groups entries that share the same base name into a folder node
   - Has no concept of functional blocks, sentinels, texture pairs, or particle FX groups
   - Produces a flat, poorly-organized tree

The lambda needs to be **deleted entirely** and replaced by `WadNodeBuilder::Build()`.

### 2.3 `ParsedEntry` Current Fields

```cpp
struct ParsedEntry {
    std::string           name;         // raw name from FileDesc
    std::string           wadName;      // filename of parent WAD
    WadAssetName          parsedName;   // parsed via WadAssetName::Parse()
    uint32_t              size;
    uint32_t              offset;
    uint64_t              hash;
    std::string           schemaType;   // from GOWRTypeToString(type field)
    std::vector<ParsedEntry> children;
    std::shared_ptr<GOW::NodeInstance> instance;
};
```

---

## 3. Target Architecture

### 3.1 Overview

```
ProfileGOWR::ParseWad()
  │
  ├── [existing] decompress LZ4
  ├── [existing] read WTOC header
  ├── [existing] read GOWRFileDesc array
  ├── [existing] run blockBitSet offset resolution → absOffsets[]
  │
  └── [NEW] WadNodeBuilder builder;
             builder.Build(fileDescs, absOffsets, outWad)
                │
                ├── Pass 1: Classify()      → assign WadEntryRole to each RawEntry
                ├── Pass 2: Pair()          → merge GPU+CPU texture pairs, fold DCClientGUID
                ├── Pass 3: GroupByBlock()  → assemble functional blocks into tree
                └── Pass 4: Finalize()      → set displayName, sort children
                │
                └── populates outWad.entries (the final ParsedEntry tree)
```

### 3.2 Internal `RawEntry` Struct (builder-private)

The builder works on a vector of `RawEntry` — a flat, mutable working set derived
directly from `GOWRFileDesc`. This is **never exposed outside the builder**.

```cpp
struct RawEntry {
    // From FileDesc
    std::string   name;
    uint32_t      size;
    uint32_t      offset;
    uint16_t      group;
    uint16_t      type;
    uint8_t       blockBitSet;

    // Derived / builder state
    WadEntryRole  role    = WadEntryRole::Unknown;
    WadBlock      block   = WadBlock::Assets;
    std::string   displayName;
    bool          consumed = false;   // true = already folded into a parent
    int           pairIdx  = -1;      // index of paired entry (texture GPU↔CPU)
};
```

---

## 4. Data Model Changes

### 4.1 Changes to `WadTypes.h`

Add two new enums **before** the `ParsedEntry` struct. Add two new fields to
`ParsedEntry`. Everything else in the file is **unchanged**.

#### 4.1.1 `WadEntryRole` enum

```cpp
// Semantic role of a single FileDesc entry inside a Ragnarök WAD.
// Derived from name patterns + type field during WadNodeBuilder::Pass1.
enum class WadEntryRole {
    // ── Structural / Meta ─────────────────────────────
    WadIdentity,       // WAD_R_* (the WAD's own identity entry)
    SharedWadRef,      // *X_R_* entries → resolve to *X_Shared_* WADs
    Sentinel,          // PopHeap, autopad (no content, structural only)

    // ── Shaders ───────────────────────────────────────
    ShaderContainer,   // 0xHASH entries (shader container descriptor)
    ShaderVertex,      // name_vs_FLAGS or HASH_vs_FLAGS
    ShaderPixel,       // HASH_ps_FLAGS

    // ── Core Assets ───────────────────────────────────
    AnimClip,          // ANM_*
    TextureGpu,        // TX_*  with large size (GPU compressed data)
    TextureCpu,        // TX_*  with small size (CPU descriptor/header)
    Material,          // MAT_HASH (with size > 0)
    MaterialRef,       // MAT_HASH (with size == 0, reference only)
    LodBinding,        // N_0_M entries (LOD/material binding table)
    MeshGpu,           // MG_*_gpu
    MeshDefn,          // MESH_*
    Model,             // MDL_*

    // ── Game Objects ──────────────────────────────────
    GameObjectProto,   // goProto*
    GameObjectInst,    // go* (plain instance, no suffix)
    GameObjectOverride,// go*_overrideInst

    // ── Audio ─────────────────────────────────────────
    SoundEmitter,      // SEMW_* (Wwise subsystem)

    // ── Particle FX ───────────────────────────────────
    ClientGuid,        // DCClientGUID (registration prelude, hidden in UI)
    ParticleEmitter,   // PEM_emit_*
    ParticleSystem,    // PTC_part_*

    // ── Synthetic (builder-generated) ─────────────────
    TexturePair,       // Virtual parent node wrapping GPU+CPU pair
    ShaderGroup,       // Virtual folder grouping shader variants
    FxGroup,           // Virtual folder grouping one PEM+PTC+go context
    ManifestBlock,     // Virtual folder for the Manifest section
    ShaderBlock,       // Virtual folder for the Shaders section
    AssetBlock,        // Virtual folder for the Assets section
    ParticleBlock,     // Virtual folder for the Particles section

    Unknown
};
```

#### 4.1.2 `WadBlock` enum

```cpp
// Functional block that an entry belongs to, as observed in the WAD layout.
enum class WadBlock {
    Manifest,   // entries from WAD start up to and including PopHeap
    Shaders,    // shader entries (vs/ps + containers) after PopHeap
    Assets,     // textures, materials, mesh, model, game objects, audio
    Particles,  // PEM/PTC/go groups at the end of the WAD
    Unknown
};
```

#### 4.1.3 New fields on `ParsedEntry`

Add these two fields to the existing `ParsedEntry` struct:

```cpp
struct ParsedEntry {
    // ... all existing fields unchanged ...

    // NEW: semantic classification (set by WadNodeBuilder)
    WadEntryRole  role        = WadEntryRole::Unknown;
    WadBlock      block       = WadBlock::Unknown;
    std::string   displayName;  // human-friendly name for UI (may differ from `name`)
};
```

The `displayName` field is what `WadBrowser` should prefer to show (the builder
sets it; if empty, fall back to `name`). **Do not change `WadBrowser.cpp`** —
it already uses `entry.name` for display; the new field is opt-in for future use.

---

## 5. New File: `WadNodeBuilder.h`

**Location:** `src/core/profiles/gowr/WadNodeBuilder.h`

```cpp
#pragma once
#include \"core/WadTypes.h\"
#include <vector>
#include <string>
#include <cstdint>
#include <span>

// Forward declaration of the internal FileDesc struct defined in ProfileGOWR.cpp
namespace GOW { struct GOWRFileDesc; }

namespace GOW {

// ── WadNodeBuilder ────────────────────────────────────────────────────────────
//
// Converts a flat array of GOWRFileDesc entries (from a parsed WTOC WAD) into
// a hierarchical ParsedEntry tree stored in outWad.entries.
//
// The builder runs four passes:
//   Pass 1 — Classify:    assign WadEntryRole + WadBlock to each raw entry
//   Pass 2 — Pair:        merge GPU+CPU texture pairs; fold DCClientGUID
//   Pass 3 — GroupByBlock: assemble the four functional blocks
//   Pass 4 — Finalize:    set displayName, sort children
//
// All logic is isolated here. ProfileGOWR::ParseWad only needs to call Build().

class WadNodeBuilder {
public:
    // Main entry point. Call once after absOffsets are computed.
    // Populates outWad.entries with the final tree.
    void Build(
        const std::vector<GOWRFileDesc>& descs,
        const std::vector<size_t>&       absOffsets,
        const std::string&               wadFilename,
        OpenWad&                         outWad);

private:
    // Internal working set — populated from descs in Build()
    struct RawEntry {
        std::string   name;
        uint32_t      size;
        uint32_t      offset;
        uint16_t      group;
        uint16_t      type;
        uint8_t       blockBitSet;
        std::string   schemaType;

        WadEntryRole  role     = WadEntryRole::Unknown;
        WadBlock      block    = WadBlock::Unknown;
        std::string   displayName;
        bool          consumed = false;
        int           pairIdx  = -1;
    };

    std::vector<RawEntry> m_entries;
    std::string           m_wadFilename;

    // The four passes
    void Pass1_Classify();
    void Pass2_Pair();
    void Pass3_GroupByBlock(OpenWad& outWad);
    void Pass4_Finalize(OpenWad& outWad);

    // Helpers
    static WadEntryRole  ClassifyByName(const std::string& name, uint32_t size);
    static WadBlock      DetermineBlock(size_t entryIndex,
                                        const std::vector<RawEntry>& entries);
    static std::string   MakeSharedWadName(const std::string& entryName);
    static ParsedEntry   ToNode(const RawEntry& r, const std::string& wadFilename);
    static bool          IsShaderName(const std::string& name);
    static bool          IsHexHash(const std::string& s);
    static std::string   ExtractFxContext(const std::string& entryName);
};

} // namespace GOW
```

---

## 6. New File: `WadNodeBuilder.cpp` — The Four Passes

**Location:** `src/core/profiles/gowr/WadNodeBuilder.cpp`

### 6.1 `Build()` — Entry Point

```cpp
void WadNodeBuilder::Build(
    const std::vector<GOWRFileDesc>& descs,
    const std::vector<size_t>&       absOffsets,
    const std::string&               wadFilename,
    OpenWad&                         outWad)
{
    m_wadFilename = wadFilename;
    m_entries.clear();
    m_entries.reserve(descs.size());

    for (size_t i = 0; i < descs.size(); ++i) {
        RawEntry r;
        r.name        = std::string(descs[i].name,
                            strnlen(descs[i].name, sizeof(descs[i].name)));
        r.size        = descs[i].size;
        r.offset      = static_cast<uint32_t>(absOffsets[i]);
        r.group       = descs[i].group;
        r.type        = descs[i].type;
        r.blockBitSet = descs[i].blockBitSet;
        r.schemaType  = GOWRTypeToString(descs[i].type);  // existing helper
        m_entries.push_back(std::move(r));
    }

    Pass1_Classify();
    Pass2_Pair();
    Pass3_GroupByBlock(outWad);
    Pass4_Finalize(outWad);
}
```

### 6.2 Pass 1 — Classify

**Goal:** assign `role` and `block` to every `RawEntry`.

**Block detection strategy:** scan linearly. The `Manifest` block is everything
from index 0 up to and including the first `PopHeap` entry. The `Shaders` block
follows until either another `PopHeap`/`autopad` ends it or the first non-shader
entry appears. From there: `Assets` until the first `PEM_emit_` entry, then
`Particles` to the end.

**Role classification rules** (in priority order):

```
name == \"PopHeap\"              → Sentinel,       block=Manifest
name == \"autopad\"              → Sentinel,       block=(current)
name starts with \"WAD_R_\" AND
  name ends with WAD filename base  → WadIdentity
name matches *X_R_* pattern    → SharedWadRef
name starts with \"0x\" OR
  (IsHexHash(name) AND no \"_vs_\"/\"_ps_\" suffix)  → ShaderContainer
name contains \"_vs_\"           → ShaderVertex
name contains \"_ps_\"           → ShaderPixel
name starts with \"depth_vs_\" OR
  \"depvl_vs_\" OR \"opaque_vs_\" OR \"transp_vs_\"  → ShaderVertex
name starts with \"ANM_\"        → AnimClip
name starts with \"TX_\"  AND size >= 1024  → TextureGpu
name starts with \"TX_\"  AND size < 1024   → TextureCpu
name starts with \"MAT_\" AND size > 0      → Material
name starts with \"MAT_\" AND size == 0     → MaterialRef
name matches /^\\d+_\\d+_\\d+$/  → LodBinding   (e.g. \"0_0_1\", \"3_0_21\")
name starts with \"MG_\" AND name ends with \"_gpu\"  → MeshGpu
name starts with \"MESH_\"       → MeshDefn
name starts with \"MDL_\"        → Model
name starts with \"goProto\"     → GameObjectProto
name ends with \"_overrideInst\" → GameObjectOverride
name starts with \"go\" (lowercase, no other prefix) → GameObjectInst
name starts with \"SEMW_\"       → SoundEmitter
name == \"DCClientGUID\"         → ClientGuid
name starts with \"PEM_emit_\"   → ParticleEmitter
name starts with \"PTC_part_\"   → ParticleSystem
everything else                → Unknown
```

**SharedWadRef detection:** An entry is a `SharedWadRef` if its name matches the
pattern `[A-Z]+X_R_[A-Za-z0-9]+` — e.g. `TXRX_R_Fox00`, `ANMX_R_Fox00`.
Regex equivalent: `^[A-Z]+X_R_`.

**Block assignment:** implement a simple state machine that tracks the current
block as you scan entries left to right:

```
state = Manifest
for each entry i:
  if state == Manifest:
    entry.block = Manifest
    if entry.role == Sentinel AND entry.name == \"PopHeap\":
      state = Shaders
  elif state == Shaders:
    entry.block = Shaders
    if entry.role NOT IN {ShaderContainer, ShaderVertex, ShaderPixel, Sentinel}:
      state = Assets
      entry.block = Assets
  elif state == Assets:
    entry.block = Assets
    if entry.role == ParticleEmitter:
      state = Particles
      entry.block = Particles
  elif state == Particles:
    entry.block = Particles
```

### 6.3 Pass 2 — Pair

**Goal:** mark texture GPU/CPU pairs and fold `DCClientGUID` entries.

**Texture pairing:** scan the entries. For each `TextureGpu` entry at index `i`,
look forward for a `TextureCpu` entry with the **exact same name**. When found at
index `j`:
- Set `m_entries[i].pairIdx = j`
- Set `m_entries[j].consumed = true` (the CPU entry will be a child, not a root)

Keep the GPU entry as the \"primary\" (`consumed = false`). During Pass 3, when
building the tree from a `TextureGpu` entry, also attach the paired `TextureCpu`
as a child node with `displayName = \"[CPU descriptor]\"`.

**DCClientGUID folding:** for each `ClientGuid` entry at index `i`, find the next
non-`ClientGuid`, non-`Sentinel` entry at index `j > i`. Mark
`m_entries[i].consumed = true`. The GUID is hidden — it is internal engine
plumbing and adds no value in the UI. The asset at `j` stands on its own.

> **Why not show DCClientGUID?** It is an internal registration handle used by
> `wadContext::Enqueue`. It carries no human-readable content (sz=37–74 bytes of
> binary GUID data). Showing it would clutter the tree with noise.

### 6.4 Pass 3 — GroupByBlock

**Goal:** build the actual `ParsedEntry` tree in `outWad.entries`.

Structure the output as **four top-level folder nodes**, each containing the
entries for that block. If a block has zero non-consumed entries, omit it.

```
outWad.entries:
  [0] ParsedEntry  name=\"Manifest\"   role=ManifestBlock  children=[...]
  [1] ParsedEntry  name=\"Shaders\"    role=ShaderBlock    children=[...]
  [2] ParsedEntry  name=\"Assets\"     role=AssetBlock     children=[...]
  [3] ParsedEntry  name=\"Particles\"  role=ParticleBlock  children=[...]
```

**Manifest block children:** one child per non-consumed entry in `WadBlock::Manifest`.
- `WadIdentity` entries → direct children, `displayName = name`
- `SharedWadRef` entries → direct children with `displayName` showing the resolved
  shared WAD name. Example: `ANMX_R_Fox00` → `displayName = \"ANMX → ANMX_Shared_Fox00\"`
  Use `MakeSharedWadName()` helper.
- `Sentinel` entries (`PopHeap`) → **omit** from output (internal structure only)

**Shaders block children:** group shader entries into sub-folders by shader stage:

```
Shaders/
  [Vertex Shaders]/
    depth_vs (12 variants as children)
    depvl_vs (12 variants)
    opaque_vs (12 variants)
    transp_vs (4 variants)
    HASH_vs_* (per-material vertex shaders)
  [Pixel Shaders]/
    HASH_ps_* (per-material pixel shaders)
  [Containers]/
    0xHASH entries
  autopad → omit
```

Group `ShaderVertex` entries by their **name prefix before `_vs_`**. Entries that
share the same prefix (e.g. `depth_vs_00000216` and `depth_vs_00001216`) become
children of a synthetic group node named `depth_vs` with `role=ShaderGroup`.

For `ShaderPixel` entries, group by the **hash prefix** (the part before `_ps_`).
All `04bad27ce70ee0d6_ps_*` variants become children of a `04bad27ce70ee0d6`
group node with `role=ShaderGroup`.

**Assets block children:** no sub-grouping by default. Each non-consumed entry
becomes a direct child. Special handling:

- `TextureGpu` entries: create a parent node with `role=TexturePair`,
  `displayName = TX_name_HASH (stripped)`, two children: `[GPU]` and `[CPU]`.
  The stripped display name should remove the long content hash from the end.
  Example: `TX_angrboda_fox00_head_gen_0d_1D293ECA4DE04637` →
  display as `TX_angrboda_fox00_head_gen_0d`.
  
- `LodBinding` entries: group all `N_0_M` entries under a single synthetic
  `[LOD Bindings]` folder node with `role=LodBinding`.

- `MaterialRef` entries (sz=0): these are back-references from particle FX to
  materials. They appear between `PTC_part_*` and `DCClientGUID` entries in the
  Assets block. Include them as direct children with `displayName = \"→ \" + name`
  to visually indicate they are references.

**Particles block children:** group by FX context using `ExtractFxContext()`.

`ExtractFxContext()` logic: given an entry name like
`goProtofox00_envRaceIntro_dust_landing`, extract the context string
`envRaceIntro_dust_landing`. More precisely: strip the leading `go*/goProto*/PEM_emit_*/PTC_part_*/` prefix, then take the remainder. If no recognizable prefix is found, use the full name.

Group entries that share the same extracted context into a synthetic `FxGroup`
node. Name the group after the context string. Example:

```
Particles/
  [envRaceIntro_dust_landing]/
    PEM_emit_dust_burst_thick
    PEM_emit_bits_filler
    PEM_emit_debris_bits
    PTC_part_dust_burst_thick
    PTC_part_bits_filler
    PTC_part_debris_bits
    goProtofox00_envRaceIntro_dust_landing
    gofox00_envraceintro_dust_landing
  [envRaceIntro_dust_takeoff]/
    ...
```

For particle entries that don't share a context with others (singletons), include
them as direct children of the `Particles` block folder.

### 6.5 Pass 4 — Finalize

**Goal:** set `displayName` for any entry that hasn't had one set yet; apply
consistent sorting within each group.

**Display name rules:**
- If `displayName` is already set → leave it
- `SharedWadRef` → `\"[subsystem] → shared_name\"` (set in Pass 3, but verify here)
- `TexturePair` → strip content hash from end:
  `TX_angrboda_fox00_head_gen_0d_1D29...` → `TX_angrboda_fox00_head_gen_0d`
- `ShaderGroup` → the group prefix (e.g. `depth_vs`, `04bad27ce70ee0d6`)
- `FxGroup` → the context string
- All others → `name` (raw name from FileDesc)

**Sorting:**
- Within `Shaders/[Vertex Shaders]`: sort groups alphabetically, variants within
  groups by the FLAGS suffix value
- Within `Assets`: textures first (sorted by base name), then materials, then
  mesh/model, then game objects, then sound
- Within `Particles`: `FxGroup` folders sorted alphabetically
- Within each `FxGroup`: `ParticleEmitter` first, then `ParticleSystem`, then
  `MaterialRef`, then `GameObjectProto`, then `GameObjectInst`

---

## 7. Modifying `ProfileGOWR.cpp`

### 7.1 What to Remove

Delete the entire `getBaseGroupName` lambda (approximately 40 lines starting with
`auto getBaseGroupName = [](const std::string& name) -> std::string {`).

Delete the \"Pass 1: Categorize entries\" and \"Pass 2: Assemble Tree\" sections that
follow it (another ~40 lines).

Delete the `std::sort` call at the end of those passes.

### 7.2 What to Add

Add `#include \"WadNodeBuilder.h\"` at the top of `ProfileGOWR.cpp`.

After the `absOffsets` vector is fully computed (after the final flush loop), replace
the deleted code with:

```cpp
    WadNodeBuilder builder;
    builder.Build(fileDescs, absOffsets, outWad.filename, outWad);
    
    LOG_INFO(\"[GOWR] Parsed WAD: %u entries → %zu root nodes.\",
        header.fileCount, outWad.entries.size());
    return true;
```

### 7.3 `GOWRFileDesc` Accessibility

`GOWRFileDesc` is currently defined inside `ProfileGOWR.cpp` inside the `GOW`
namespace under a `#pragma pack(push, 1)`. Move it to a new small header
`GOWRTypes.h` in `src/core/profiles/gowr/` so that `WadNodeBuilder.cpp` can
include it.

Alternatively, keep it in `ProfileGOWR.cpp` and forward-declare it in
`WadNodeBuilder.h` — but the cleanest approach is the separate header.

**`GOWRTypes.h` content:**

```cpp
#pragma once
#include <cstdint>

namespace GOW {

// Type string mapping — also used by WadNodeBuilder
std::string GOWRTypeToString(uint16_t type);

#pragma pack(push, 1)
struct GOWRWadHeader {
    uint32_t magic;
    uint32_t ver;
    uint32_t fileCount;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t block0Size;
    uint32_t block1Size;
    uint32_t block2Size;
    uint8_t  unk3[20];
    uint32_t block8Size;
    uint32_t unk4;
    uint32_t unk5;
};

struct GOWRFileDesc {
    uint16_t group;
    uint16_t type;
    uint32_t size;
    uint8_t  unk1[16];
    char     name[0x38];
    uint8_t  unk2[0x1F];
    uint8_t  blockBitSet;
    uint8_t  unk3[0x8];
    uint32_t offset;
    uint8_t  unk4[12];
    uint32_t offset2;
    uint8_t  unk5[4];
};
#pragma pack(pop)

} // namespace GOW
```

Move the `GOWRTypeToString()` implementation out of `ProfileGOWR.cpp` into a new
`GOWRTypes.cpp` file, or keep it in `ProfileGOWR.cpp` and just declare it in the
header. Either is acceptable.

---

## 8. Updating `UIHelpers.h`

`ColorForType()` and `IconForType()` currently key off `schemaType` (the `type`
field string from `GOWRTypeToString`). They need to also understand `WadEntryRole`
so that synthetic nodes (block folders, shader groups, texture pairs, FX groups)
render with appropriate visuals.

Add two new overloads that accept `WadEntryRole`:

```cpp
inline ImVec4 ColorForRole(WadEntryRole role) {
    switch (role) {
        case WadEntryRole::ManifestBlock:
        case WadEntryRole::WadIdentity:       return {0.9f, 0.9f, 0.9f, 1.0f}; // white
        case WadEntryRole::SharedWadRef:      return {0.8f, 0.8f, 1.0f, 1.0f}; // light blue
        case WadEntryRole::Sentinel:          return {0.4f, 0.4f, 0.4f, 1.0f}; // dark gray (hidden)
        case WadEntryRole::ShaderBlock:
        case WadEntryRole::ShaderGroup:
        case WadEntryRole::ShaderVertex:
        case WadEntryRole::ShaderPixel:       return {0.5f, 1.0f, 0.5f, 1.0f}; // green
        case WadEntryRole::ShaderContainer:   return {0.4f, 0.8f, 0.4f, 1.0f}; // darker green
        case WadEntryRole::AnimClip:          return {1.0f, 0.8f, 0.3f, 1.0f}; // yellow
        case WadEntryRole::TexturePair:
        case WadEntryRole::TextureGpu:
        case WadEntryRole::TextureCpu:        return {1.0f, 0.5f, 0.8f, 1.0f}; // pink
        case WadEntryRole::Material:
        case WadEntryRole::MaterialRef:       return {0.9f, 0.6f, 0.2f, 1.0f}; // orange
        case WadEntryRole::LodBinding:        return {0.6f, 0.6f, 0.6f, 1.0f}; // gray
        case WadEntryRole::MeshGpu:
        case WadEntryRole::MeshDefn:          return {0.4f, 0.8f, 1.0f, 1.0f}; // blue
        case WadEntryRole::Model:             return {0.5f, 0.9f, 1.0f, 1.0f}; // light blue
        case WadEntryRole::GameObjectProto:
        case WadEntryRole::GameObjectInst:
        case WadEntryRole::GameObjectOverride:return {1.0f, 0.7f, 0.7f, 1.0f}; // salmon
        case WadEntryRole::SoundEmitter:      return {0.3f, 0.9f, 0.6f, 1.0f}; // teal
        case WadEntryRole::ParticleBlock:
        case WadEntryRole::FxGroup:
        case WadEntryRole::ParticleEmitter:
        case WadEntryRole::ParticleSystem:    return {1.0f, 0.6f, 0.9f, 1.0f}; // lavender
        case WadEntryRole::ClientGuid:        return {0.3f, 0.3f, 0.3f, 0.5f}; // very dim
        default:                             return {0.6f, 0.6f, 0.6f, 1.0f};
    }
}

inline const char* IconForRole(WadEntryRole role) {
    // Using codicon codepoints embedded as UTF-8
    switch (role) {
        case WadEntryRole::ManifestBlock:
        case WadEntryRole::ShaderBlock:
        case WadEntryRole::AssetBlock:
        case WadEntryRole::ParticleBlock:
        case WadEntryRole::ShaderGroup:
        case WadEntryRole::FxGroup:           return \"\\xEE\\xA9\\x83\"; // folder
        case WadEntryRole::SharedWadRef:      return \"\\xEE\\xA9\\x86\"; // folder-opened (link)
        case WadEntryRole::WadIdentity:       return \"\\xEE\\xA9\\xBB\"; // file
        case WadEntryRole::AnimClip:          return \"\\xEE\\xA8\\x9F\"; // play
        case WadEntryRole::TexturePair:
        case WadEntryRole::TextureGpu:
        case WadEntryRole::TextureCpu:        return \"\\xEE\\xA8\\xBB\"; // file-media
        case WadEntryRole::Material:
        case WadEntryRole::MaterialRef:       return \"\\xEE\\xA8\\xBB\"; // symbol-color
        case WadEntryRole::MeshGpu:
        case WadEntryRole::MeshDefn:
        case WadEntryRole::Model:             return \"\\xEE\\xA8\\xBB\"; // symbol-misc (cube)
        case WadEntryRole::ShaderVertex:
        case WadEntryRole::ShaderPixel:
        case WadEntryRole::ShaderContainer:   return \"\\xEE\\xA8\\xBB\"; // code
        case WadEntryRole::GameObjectProto:
        case WadEntryRole::GameObjectInst:
        case WadEntryRole::GameObjectOverride:return \"\\xEE\\xA8\\xBB\"; // person
        case WadEntryRole::SoundEmitter:      return \"\\xEE\\xA8\\xBB\"; // unmute
        case WadEntryRole::ParticleEmitter:
        case WadEntryRole::ParticleSystem:    return \"\\xEE\\xA8\\xBB\"; // sparkle
        case WadEntryRole::LodBinding:        return \"\\xEE\\xA8\\xBB\"; // list-tree
        case WadEntryRole::Sentinel:          return \"\\xEE\\xA8\\xBB\"; // dash
        case WadEntryRole::ClientGuid:        return \"\\xEE\\xA8\\xBB\"; // key
        default:                             return \"\\xEE\\xA9\\xBB\"; // file
    }
}
```

Update `ColorForType()` and `IconForType()` to delegate to the role-based versions
when `entry.role != WadEntryRole::Unknown`. The `WadBrowser` uses these helpers —
since we're not changing `WadBrowser.cpp`, we must update the helpers to keep the
existing call-sites working. The best approach: update `WadBrowser.cpp`'s call
from `ColorForType(entry.schemaType)` to `ColorForRole(entry.role)` — **wait**,
the constraint says don't change `WadBrowser.cpp`. Instead, keep both functions
working; the builder should set `schemaType` to a string that `ColorForType` still
handles, AND set `role` for use by future code.

**Pragmatic solution:** add the new `ColorForRole` / `IconForRole` overloads to
`UIHelpers.h`, and also update the existing `ColorForType` / `IconForType` to
check for GOWR-specific role-hinting schemaTypes:

```cpp
// Synthetic schemaTypes set by WadNodeBuilder for virtual nodes:
// \"GOWR_BLOCK_MANIFEST\", \"GOWR_BLOCK_SHADERS\", \"GOWR_BLOCK_ASSETS\",
// \"GOWR_BLOCK_PARTICLES\", \"GOWR_SHADER_GROUP\", \"GOWR_TEXTURE_PAIR\",
// \"GOWR_FX_GROUP\", \"GOWR_LOD_BINDING_TABLE\", \"GOWR_SHARED_WAD_REF\"
```

The builder sets `schemaType` on synthetic nodes to one of these strings, and
`ColorForType` / `IconForType` get new `if` branches for them. This way
`WadBrowser` continues to call the same functions with `entry.schemaType` and gets
the right colors without any changes to `WadBrowser.cpp`.

---

## 9. File Locations Summary

| File | Action |
|---|---|
| `src/core/WadTypes.h` | Add `WadEntryRole` enum, `WadBlock` enum, two new fields to `ParsedEntry` |
| `src/UIHelpers.h` | Add `ColorForRole()`, `IconForRole()`; extend `ColorForType()`, `IconForType()` with new schemaType strings |
| `src/core/profiles/gowr/GOWRTypes.h` | **NEW** — move `GOWRWadHeader`, `GOWRFileDesc`, declare `GOWRTypeToString` |
| `src/core/profiles/gowr/WadNodeBuilder.h` | **NEW** — class declaration |
| `src/core/profiles/gowr/WadNodeBuilder.cpp` | **NEW** — four-pass implementation |
| `src/core/profiles/gowr/ProfileGOWR.cpp` | Remove `getBaseGroupName` + old tree-building code; add `WadNodeBuilder::Build()` call; add `#include \"GOWRTypes.h\"` and `#include \"WadNodeBuilder.h\"` |
| `src/core/WadAssetName.cpp` | No changes needed (already exists; `WadNodeBuilder` may use `WadAssetName::Parse()` internally if useful) |

`CMakeLists.txt` uses `file(GLOB_RECURSE SOURCES src/*.cpp)` — new `.cpp` files
under `src/` are picked up automatically. **No changes to `CMakeLists.txt` needed.**

---

## 10. What NOT To Change

These files must remain **byte-for-byte identical** after the implementation:

- `src/core/AssetDatabase.h` / `.cpp`
- `src/core/interfaces/IGameProfile.h`
- `src/core/loaders/IAssetLoader.h`
- `src/core/loaders/GOWRLoaders.h` / `.cpp`
- `src/core/loaders/GOW2Loaders.h` / `.cpp`
- `src/ui/WadBrowser.h` / `.cpp`
- `src/ui/Inspector.h` / `.cpp`
- `src/ui/ViewerRegistry.h` / `.cpp`
- `src/core/profiles/gowr/ProfileGOWR.h` (signature unchanged)
- Any GOW1/GOW2 profile files
- `src/core/WadAssetName.cpp` (unchanged; may be used but not modified)

---

## 11. Reference: Confirmed Binary Facts

This section records what was confirmed via Ghidra analysis of `GoWR.exe` and
direct binary inspection of `r_fox00.wad`. Use this to resolve any ambiguity
during implementation.

### 11.1 FileDesc Confirmed Layout

```
stride = 0x90 (144 bytes) — confirmed by scanning for repeated \"WAD_R_Fo\" pattern
name field at +0x48 — confirmed (56 bytes, null-padded)
group at +0x34 (u16) — confirmed
type  at +0x36 (u16) — confirmed, read by wadContext::GetTypeId (vtable +0x40)
blockBitSet at +0x44 — confirmed
```

### 11.2 Known Type Values

| Value | Name | Description |
|---|---|---|
| `0x0000` | `GOWR_MESH_MAP` | (unconfirmed) |
| `0x0001` | `GOWR_MESH_DEFN` | Mesh definition |
| `0x000B` | `GOWR_RIGID_MESH` | Rigid mesh |
| `0x000C` | `GOWR_SMSH_DEFN` | Static mesh definition |
| `0x0008` | `GOWR_ANIMATION` | Animation (observed in r_fox00: ANM_fox00 has type=8) |
| `0x0015` | `GOWR_MG_DEFN` | Mesh group definition (observed: MG_fox00_0 has grp flags) |
| `0x003D` | `GOWR_RIG` | Skeleton/rig |
| `0x003F` | `GOWR_GOPROTO_RIG` | Game object prototype rig |
| `0x0040` | `GOWR_JOINT_MAP` | Joint mapping |
| `0x0041` | `GOWR_ANIMATION` | Animation clip |
| `0x0042` | `GOWR_ANM_CLIP` | Animation clip (variant) |
| `0x0050` | `GOWR_COLLISION` | Collision data |
| `0x0051` | `GOWR_PHYS_DATA` | Physics data |
| `0x008F` | `GOWR_MODEL_INSTANCE` | Model instance |
| `0x0090` | `GOWR_SKIN_INSTANCE` | Skin instance |
| `0x0098` | `GOWR_SKINNED_MESH` | Skinned mesh |
| `0x0099` | `GOWR_MG_DEFN` | Mesh group definition |
| `0x801E` | `GOWR_SHADER` | Shader binary (magic `\"OrbShdr\"` at `size-28`) |
| `0x8020` | `GOWR_MATERIAL` | Material |
| `0x80A1` | `GOWR_TEXTURE_OLD` | Old texture format |
| `0x80A2` | `GOWR_TEXTURE` | Texture (GNF/PS4 format) |
| `0x8198` | `GOWR_SKINNED_MESH_BUFF` | Skinned mesh GPU buffer |
| `0x8199` | `GOWR_MG_GPU_BUFF` | Mesh group GPU buffer |

> **Note:** There is no central switch/table for type names in the binary. The
> engine uses polymorphism per-subsystem. The table above is derived from
> reverse engineering and empirical observation. Type 0 is used for many
> \"container\" or \"index\" entries whose actual format is determined by name prefix.

### 11.3 FNV Hash Variant (for reference)

Used by the engine for all name-based lookups:
```c
uint32_t FNVHash(const char* str) {
    uint32_t hash = 0;
    for (each char c) {
        uint8_t adjusted = (c < 0x20) ? (c - 0x20) & 0xFF : c;
        hash = (hash + adjusted) * 0x401 ^ ((hash + adjusted) * 0x401 >> 6);
    }
    return hash;
}
```
Prime: `0x401` = 1025. Mix step: XOR with right-shift-6. **Not standard FNV-1a.**

### 11.4 SharedWadRef Confirmed Behavior

`FUN_14052f270` in `GoWR.exe`:
```c
FUN_140532160(buf, \"MATX_Shared_%s\", wadBaseName);   // flags = 0x9000000a
FUN_140532160(buf, \"ANMX_Shared_%s\", wadBaseName);   // flags = 0x90000002
```
The `*X_R_*` entries in the WAD trigger lookup of `*X_Shared_*` named assets via
`AssetRegistry_HashLookup`. These are independently loaded WADs, not embedded
sub-WADs.

### 11.5 wadContext Class Hierarchy

```
wadServerBase
  └── wadServer
        └── wadContext             (14 virtual functions)
              └── WwiseWadContext  (audio subsystem subclass, handles SEMW_* entries)
wcUnloadActionList                 (*X_R_* entries register into this on unload)
wadUnloadParm
```

Key vtable slots of `wadContext` @ `0x141d7eac8`:
- `+0x10` — drain/flush unload queue (iterates linked list of entries)
- `+0x18` — `Dequeue` / pop entry from loaded list; decrements count; calls vtable[+0x70] (AddRef/Release)
- `+0x28` — `Enqueue` / push entry to loaded list; sets `entry[4] &= ~1` (loaded flag)
- `+0x30` — `Init` / full constructor (`FUN_140546aa0`)
- `+0x40` — `GetTypeId` — reads `u16` from entry `+0x14`, writes to destination `+0x06`

### 11.6 WAD Category Prefix Table (`FUN_140545820`)

```
PTR_s_WAD_R_W__141d7e950:
  \"WAD_R_W_\"  len=8  flag=1   → World WAD
  \"WAD_R_I_\"  len=8  flag=1   → Interactive WAD
  \"WAD_R_\"    len=6  flag=0   → Generic Ragnarök WAD
  \"WAD_Char_\" len=9  flag=0   → Character WAD
  \"WAD_Add_\"  len=8  flag=0   → Additive/DLC WAD
  \"WAD_\"      len=4  flag=0   → Fallback WAD
  fallback    idx=6  flag=0   → Unknown
```

High-priority flag `0x10` set for: `WAD_R_HeroA00`, `WAD_R_Atreus00`,
`WAD_R_FreyaValkyrie00`, `WAD_R_UI`, `WAD_Rbr_HomeBase200_SindriHouseInt`.

---

*End of spec. Implement incrementally: start with `WadTypes.h` additions, then
`GOWRTypes.h` extraction, then `WadNodeBuilder` skeleton + Pass1, compile, then
add Pass2–4 one at a time. The builder is completely independent — `ProfileGOWR`
only changes at the very end when calling `Build()`.*