# God of War: Ragnarök — Internal Systems Documentation
> Compiled from Ghidra reverse engineering, RTTI analysis, vtable reconstruction, and tool source code (C++ extractor + C# GoW2018 predecessor).  
> All addresses reference the PC/PS4 binary. Confidence levels: ✅ confirmed | ⚠️ inferred | ❓ unknown.

---

## Table of Contents

1. [File System & Asset Pipeline](#1-file-system--asset-pipeline)
2. [WAD Archive Format](#2-wad-archive-format)
3. [Lodpack Format](#3-lodpack-format)
4. [Texture Pipeline (Texpack / GNF)](#4-texture-pipeline-texpack--gnf)
5. [Mesh Format (MESH_DEFN / MG)](#5-mesh-format-mesh_defn--mg)
6. [Rig / Skeleton Format](#6-rig--skeleton-format)
7. [Animation System (`anm`)](#7-animation-system-anm)
   - 7.1 [Class Hierarchy](#71-class-hierarchy)
   - 7.2 [anmNode — Base Node](#72-anmnode--base-node)
   - 7.3 [anmRootNode — Root of the Blend Graph](#73-anmrootnode--root-of-the-blend-graph)
   - 7.4 [anmDataAnimator — Data Channel Base](#74-anmdataanimator--data-channel-base)
   - 7.5 [anmMatrixArray — Bone Matrix Node](#75-anmmatrixarray--bone-matrix-node)
   - 7.6 [AnmAsyncResolver — Async Pose Resolver](#76-anmasyncresolver--async-pose-resolver)
   - 7.7 [Layer Type Enum](#77-layer-type-enum)
   - 7.8 [World Context Table](#78-world-context-table)
8. [Rendering — ChainTrail Effect](#8-rendering--chaintail-effect)
9. [Entity & Server/Client Architecture](#9-entity--serverclient-architecture)
10. [Hash & Registry Systems](#10-hash--registry-systems)
11. [Memory & Allocator System](#11-memory--allocator-system)
12. [Known Hardcoded Character IDs](#12-known-hardcoded-character-ids)
13. [Cross-Reference: GoW 2018 vs Ragnarök](#13-cross-reference-gow-2018-vs-ragnarök)
14. [Symbol Rename Table](#14-symbol-rename-table)

---

## 1. File System & Asset Pipeline

### Overview

Assets are organized in a layered pipeline:

```
WAD Archive (.wad / .bin)
  └── FileDesc entries (typed, named, offset-mapped)
        ├── GOWR_MESH_DEFN   — mesh geometry definitions
        ├── GOWR_MESH_GPU    — GPU vertex/index buffers (lodpack-referenced)
        ├── GOWR_SHADER      — shader binaries (magic: "OrbShdr" at EOF-28)
        ├── GOWR_RIG         — skeleton definition
        ├── GOWR_ANIM        — animation clip data
        └── ... (other typed entries)

Lodpack (.lodpack + .lodpack.toc)
  └── group[] → member[]
        └── keyed by uint64 hash → raw GPU buffer data

Texpack (.texpack)
  └── TexInfo[] + BlockInfo[]
        └── GNF image blocks (PS4 GCN swizzle / PS5 RDNA)
```

### Asset Loading Entry Points (Ghidra)

| Function | Address | Role |
|---|---|---|
| `WAD_OpenPermPack` | `FUN_140429610` | Opens a permanent WAD pack by ID |
| `AssetRegistry_FindByName` | `FUN_14054d240` | Finds registry by string name (e.g. `"WAD_R_Perm"`) |
| `AssetRegistry_HashLookup` | `FUN_140548100` | Looks up an entry by FNV hash key |
| `AssetRegistry_ResolveObject` | `FUN_140429a20` | Resolves asset object from registry node |
| `GetVTable` | `FUN_140429670` | Retrieves vtable pointer for a type ID |
| `ReleaseResourceHandle` | `FUN_1406e3a70` | Releases a resource handle (ref-counted) |

---

## 2. WAD Archive Format

### Header (magic `0x434F5457` = "WTOC", version `0x2`)

```c
struct WADHeader {
    uint32_t magic;       // 0x434F5457 "WTOC"
    uint32_t version;     // must be 0x2
    uint32_t fileCount;
    uint32_t padding;
};
```

### FileDesc Entry

Each file entry contains:
- `group` (uint16), `type` (uint16) — typed enum (`GOWR_MESH_DEFN`, `GOWR_SHADER`, etc.)
- `size` (uint32), `offset` (uint32), `offset2` (uint32)
- `name[0x38]` — null-terminated ASCII name
- `blockBitSet` (uint8) — controls which "block group" this file belongs to
- `unk2[0x1F]`, `unk3[8]`, `unk4[12]`, `unk5[4]` — partially decoded flags

### Offset Resolution Algorithm ✅

The WAD uses a deferred multi-queue system to resolve absolute file offsets. Files are grouped by `blockBitSet`. Files with `unk3[2] == 1` act as **block boundaries** (flush triggers). Files with `unk2[20] != 0` are additionally queued into group 8 (secondary stream).

```
bitsetOffs[blockBitSet] = running cursor per group
flushQ[blockBitSet]     = pending file queue per group
```

When a boundary is hit, all pending queues flush their offsets using their respective cursors, then advance. This allows multiple interleaved data streams in a single file.

### Known Magic Values

| Type | Magic | Description |
|---|---|---|
| `GOWR_MESH_DEFN` (MESH_*) | `655372` | Mesh definition block |
| `GOWR_MESH_DEFN` (MG_*) | `65548` | Mesh group block |
| `GOWR_SHADER` | `"OrbShdr"` at `size-28` | Orbis (PS4) shader binary |

---

## 3. Lodpack Format

### Structure

```
Header (16 bytes):
  uint32 groupCount
  uint32 totalMemberCount
  uint64 constant (= 0x100000000)

Group table [groupCount × 24 bytes]:
  uint64 startOffset
  uint64 groupHash
  uint32 blockSize
  uint32 padding

Member table [totalMemberCount × 24 bytes]:
  uint32 groupIndex
  uint32 offsetInGroup
  uint64 memberHash       ← primary lookup key
  uint32 blockSize
  uint32 padding
```

### Lookup

`memberHash` is a `uint64` that matches a hash stored in the `MESH_DEFN` file. The C++ tool and C# tool both use the pattern:

```cpp
if (!dictionary.ContainsKey(hash)) {
    hash--;  // try hash-1 as fallback (✅ confirmed in GoW2018.cs)
}
```

This `hash - 1` fallback suggests the engine sometimes stores `hash+1` as a LOD variant key.

### Write Format

When writing, a `.toc` sidecar file is generated alongside the `.lodpack` with identical content. The data section is 16-byte aligned.

---

## 4. Texture Pipeline (Texpack / GNF)

### Texpack Structure

```
Header @ 0x20:
  uint32 texSectionOffset
  uint32 blockCount
  uint32 blockInfoOffset
  uint32 texCount

TexInfo[texCount] @ 0x38:
  uint64 fileHash
  uint64 userHash
  uint64 blockInfoOffset   ← absolute file offset of first BlockInfo

BlockInfo[blockCount]:
  int64  blockOffset       ← shifted: absolute = (blockOffset << 4) + 4
  int64  nextSiblingOff    ← -1 = no sibling (last mip block)
  uint32 rawSize
```

### Block Chain

Textures with multiple mip levels use a **singly-linked sibling chain** through `nextSiblingOff`. The chain is traversed backwards (from lowest mip to highest) to reconstruct the full mip pyramid. When `nextSiblingOff == -1`, the block is the base mip.

### GNF (PS4) Image Format

Header is 0x100 bytes. Key fields:

```c
struct GnfHeader {
    uint16_t width;      // actual = width + 1
    uint16_t height;     // actual = height + 1
    uint8_t  mipmaps;    // actual = mipmaps + 1
    uint8_t  depth;      // actual = depth + 1
    uint8_t  format;     // Gnf::Ps4::Format enum
    uint8_t  formatType; // UNORM / SNORM / SRGB / UINT / SINT
    uint16_t pitch;      // RoundUpTo2(width) - 1
    uint8_t  destX/Y/Z/W;// channel swizzle
    uint32_t dataSize;
    uint32_t fileSize;   // dataSize + 0x100
};
```

### Swizzle Algorithm ✅ (confirmed in Gnf.cpp and GoW2018.cs — identical implementations)

PS4 GCN textures use **Morton Z-order curve** tiling in 8×8 block groups:

```
for each 8×8 macro-tile (i, j):
    for k in 0..63:
        src_idx = morton(k, 8, 8)  // Z-curve index within tile
        dst_x = j*8 + (src_idx % 8)
        dst_y = i*8 + (src_idx / 8)
        copy texel
```

The `morton()` function deinterleaves X and Y bits from a linear index using alternating bit extraction. The stride per texel varies: `bpp*2` for BC formats (4×4 pixel blocks), `bpp/8` for uncompressed.

### Supported Formats

| Enum | DXGI Equivalent | bpp | pixbl |
|---|---|---|---|
| `FormatBC1` | BC1_UNORM[_SRGB] | 4 | 4 |
| `FormatBC2` | BC2_UNORM[_SRGB] | 8 | 4 |
| `FormatBC3` | BC3_UNORM[_SRGB] | 8 | 4 |
| `FormatBC4` | BC4_UNORM/SNORM | 4 | 4 |
| `FormatBC5` | BC5_UNORM/SNORM | 8 | 4 |
| `FormatBC6` | BC6H_UF16/SF16 | 8 | 4 |
| `FormatBC7` | BC7_UNORM[_SRGB] | 8 | 4 |
| `Format8` | R8_UNORM/SNORM/UINT/SINT | 8 | 1 |

BC6 has additional fields: `unk7 = 0xB6D`, `unk9 = 0xA000`.

---

## 5. Mesh Format (MESH_DEFN / MG)

### MESH_DEFN Layout

```
+0x00: uint32 magic (= 655372)
+0x0C: uint32 defOffsetsBegin
+0x10: uint32 defCount
+0x18: uint32 fileSize

Per-mesh definition (relative offsets):
  +0x10: Vec3  extent          ← half-extents of AABB
  +0x18: Vec3  origin          ← center of AABB
  meshScale = extent * 2
  meshMin   = origin - extent

  +0x28: uint32 indicesOffset
  +0x34: uint32 vertexOffset
  +0x3C: uint32 vertCount
  +0x40: uint32 faceCount
  +0x54: uint32 indCount
  +0x58: uint32 componentOffset
  +0x5C: uint32 bufferOffsetsOffset
  +0x60: uint64 meshHash        ← key into lodpack
  +0x74: uint8  bufferCount
  +0x75: uint8  indicesStride   (2 = uint16, 4 = uint32)
  +0x76: uint8  bytesPerVertex
  +0x77: uint8  componentCount
  +0x78: uint8  countComp2
  +0x79: uint8  flagOrUnk
  +0x7A: uint8  R32_UNK_Usage   (1 or 2 — selects joint unpack mode)
```

### Component Descriptor

```c
struct Component {
    uint8_t  primitiveType;  // 0=POSITION, 1=NORMALS, 2=TANGENTS,
                             // 3-6=TEXCOORD_0-3, 7=JOINTS0, 8=WEIGHTS0
    uint8_t  dataType;       // R32_FLOAT, R16_UNORM, R16_SNORM,
                             //  R10G10B10A2_TYPELESS, R8_UINT, R32_UNKNOWN
    uint8_t  elementCount;
    uint8_t  bufferIndex;
    uint32_t offset;         // byte offset within vertex stride
};
```

### Data Types

| ID | Name | Description |
|---|---|---|
| 0 | `R32_FLOAT` | Raw float |
| 1 | `R16_UNORM` | `x / 65535.0` |
| 2 | `R16_SNORM` | `(x - 32767) / 32768.0` |
| 3 | `R10G10B10A2_TYPELESS` | SNORM: `(x - 511) / 512.0`, UNORM: `x / 1023.0` |
| 4 | `R8_UINT` | Raw byte |
| 5 | `R32_UNKNOWN` | Custom packed format — see Joint Packing |

### Joint Packing Modes (`R32_UNKNOWN`)

**Mode 1** (`R32_UNK_Usage == 1`) — pairs of uint16:
```
for each element: read uint16 X, uint16 Y → joints[s*2+0] = X, joints[s*2+1] = Y
```

**Mode 2** (`R32_UNK_Usage == 2`) — 11-bit packed across uint32 boundaries (✅ confirmed):
```
// Up to 10 bone influences per vertex
// Packed as: 11+11+10 bits per uint32, crossing boundaries

joint[0]     = (uint32[0] >> 21) & 0x7FF
joint[1]     = (uint32[0] >> 10) & 0x7FF
joint[2]     = ((uint32[0] & 0x3FF) << 1) | (uint32[1] >> 31)
joint[3]     = (uint32[1] >> 20) & 0x7FF
joint[4]     = (uint32[1] >>  9) & 0x7FF
joint[5]     = ((uint32[1] & 0x1FF) << 2) | (uint32[2] >> 30)
...continues for each uint32
```

### Weight Packing

Weights use `R10G10B10A2_TYPELESS` (UNORM, `x / 1023.0`). Three weights per uint32 (10+10+10 bits). The last weight is computed as `1.0 - sum` to avoid storing it. Maximum 10 weights (3 uint32s + remainder).

### MG (Mesh Group) Format

```
+0x30: uint16 defCount
+0x44: uint32 defOffsets[defCount]   ← relative offsets to each MeshGroupDef

MeshGroupDef:
  +0x00: uint16 parentBone
  +0x02: uint8  lodCount
  +0x38 + j*4: uint32 lodOffset[lodCount]

LOD entry:
  +0x00: uint32 meshCount
  +0x06: uint16 meshIndices[meshCount]  ← indices into MESH_DEFN array
```

---

## 6. Rig / Skeleton Format

### Layout (from `Rig.cpp` + `GoW2018.cs`)

```
+0x10: uint16 boneCount

Bone parents array:
  +0x1E + i*8: int16 parentIndex   (-1 = root bone)

Matrix section (local-space transforms):
  baseOff = (0x18 + boneCount * 32 + 15) & ~15  (16-byte aligned)
  baseOff += 0x50
  Each bone: 4x4 float32 matrix (row-major, 64 bytes)

IBM (Inverse Bind Matrix) section:
  Immediately after matrices:
  Each bone: 4x4 float32 matrix (64 bytes)
```

### Forward Kinematics (GoW2018 implementation — identical logic expected in Ragnarök)

The GoW 2018 tool reconstructs world-space transforms via quaternion composition:

```csharp
// Local rotation matrix → quaternion (Shepperd method)
worldRot[j] = worldRot[parentIdx] * localRot[j];

// Rotate local position by parent world rotation (quaternion sandwich product)
worldPos[j] = rotate(localPos[j], worldRot[parentIdx]) + worldPos[parentIdx];
```

This is a standard FK chain using unit quaternions. The `matrix2quat` function handles the degenerate case (trace ≤ 0) by finding the largest diagonal element and computing from that axis.

---

## 7. Animation System (`anm`)

The animation system is a **runtime blend graph** evaluated per-frame. It is more sophisticated than a simple state machine — it supports Motion Matching, Inertialization, Motion Warping, RBF blending, and driven blend nodes.

### 7.1 Class Hierarchy

```
anmNode                          ← base graph node
  └── anmRootNode                ← root of the blend graph (also inherits anmClient)
        ├── anmLayerRootNode     ← per-layer root
        └── anmInertializationBlendNode

anmDataAnimator                  ← base for all data-writing nodes
  └── anmSimple<T, Interpolator> ← typed channel node
        ├── anmFloat             ← float channel (typeId=1)
        ├── anmVisible           ← bool/visibility channel (typeId=9)
        ├── anmAIGraphConnectorAnimator (typeId=0xB)
        ├── anmFlipbookAnimator  (typeId=0xC)
        └── anmUserConstAnimator (typeId=0xA) ← gameplay-driven constant
  └── anmMultiSampleTemplate<T, N, Interpolator>
        ├── <char,   2, NOINTERP> (typeId=2, stride=1)
        ├── <short,  3, NOINTERP> (typeId=3, stride=2)
        ├── <int,    4, NOINTERP> (typeId=4, stride=4)
        ├── <int64,  5, NOINTERP> (typeId=5, stride=8)
        ├── <float,  7, ScalerInterp> (typeId=7, stride=4)
        └── <FLOAT16,8, ScalerInterp> (typeId=8, stride=2)
  └── anmMatrixArray             ← bone matrix channel (typeId=0)

anmPlayer                        ← clip playback base
  └── anmSimplePlayer<T, Interpolator>
        ├── <float, ScalerInterpolator>
        ├── <FLOAT16, ScalerInterpolator>
        ├── <int64, NOINTERPOLATION>
        ├── <int, NOINTERPOLATION>
        ├── <short, NOINTERPOLATION>
        ├── <char, NOINTERPOLATION>
        └── <float, FlipbookIndexInterpolator>
  └── anmVisiblePlayer
  └── anmMatrixArrayPlayer
  └── anmMatrixArrayPlayList

Blend nodes:
  anmAdditiveBlendNode
  anmSubtractiveBlendNode
  anmDrivenBlendNode → anmBaseDrivenBlendNode → anmTwoDimensionalDrivenBlendNode
  anmAnimDrivenBlendNode
  anmFeatherBlendNode
  anmSpeedScalingBlendNode
  anmSelectionBlendNode{Int|Float|Enum}
  anmSnapshotBlendNode
  anmAmpFreqBlendNode
  anmMoodListNode
  anmRandomListNode
  anmDrivenChoiceNode
  anmMoveSystemInstanceNode
  anmPushPoseNode
  anmDummyNode

Controller nodes:
  anmTimeScaleControllerNode
  anmFreezeTimeControllerNode
  anmPostProcessControllerNode
  anmMotionWarpingControllerNode (namespace MotionWarping)
  anmMotionWarpingOffsetControllerNode
  anmSteeringControllerNode
  anmCameraDeltaControllerNode
  anmNavPelvisOffsetController
  anmVarietyControllerNode

Pose system:
  anmPoseMatchDBNode             ← Motion Matching database lookup
  anmRigNode<Client<T>>          ← template rig integration
    tPoseReader, tMultiTrigger, tDriverBank
    tDistanceNode, tTransformAttrNode
    tBlendSample, tRBFNode       ← Radial Basis Function blending
    tTransformDriver, tMaterialDriver

Infrastructure:
  anmClient, anmServer, anmServerBase, anmContext
  anmPlayList, anmMatrixArrayPlayList
  anmPlayerEndCallback
  anmUserConstAnimator
  AnmAsyncResolver               ← async pose resolution (~85KB struct)
```

### 7.2 anmNode — Base Node

**vtable address:** `0x141d719c8`  
**Constructor:** `FUN_1406ee480`  
**33 virtual slots**

#### Struct Layout

```c
struct anmNode {
    void**    vptr;            // +0x000
    uint64_t  field_0x008;     // +0x008
    uint64_t  field_0x010;     // +0x010
    // ...
    ListLink  childList;       // +0x020  circular intrusive list sentinel
                               //   .prev @ +0x020, .next @ +0x028
    // ...
    uint32_t  nameHash;        // +0x040  ─┐ FNV-32 composite key
    uint32_t  typeHash;        // +0x044   ├─ used in GetHashKey()
    uint32_t  instanceHash;    // +0x048  ─┘
    // ...
    RefObj*   ownerRef;        // +0x060  ref-counted owner
                               //   .refCount @ +0x008
                               //   .backPtr  @ +0x010 (cleared on detach)
};

struct ListLink {
    ListLink* prev;   // +0x00
    ListLink* next;   // +0x08
};
```

#### vtable Reconstruction

| Slot | Address | Name | Returns |
|---|---|---|---|
| 0 | `FUN_140479af0` | `~anmNode()` scalar dtor | — |
| 1 | `FUN_1406ee4d0` | `~anmNode()` vector dtor | — |
| 2 | `FUN_1406ee4e0` | `IsReady()` | `true` (base) |
| 3 | `FUN_14047eeb0` | `IsActive()` | `false` (base) |
| 4 | stub | — | — |
| 5 | `FUN_1406ee580` | `PropagateToChildren()` | void — recursive slot[5] call |
| 6 | stub | — | — |
| 7 | `FUN_14047c970` | `GetParent()` | `nullptr` (base) |
| 8–12 | stubs | — | — |
| 13 | `FUN_1406ee970` | `EvaluateBlend(float* outSecondary)` | `float` |
| 14–16 | stubs | — | — |
| 17 | `FUN_14047cfd0` | `GetServer()` | `nullptr` (base) |
| 18 | `FUN_14047d000` | `GetContext()` | `nullptr` (base) |
| 19 | `FUN_14047cfc0` | `GetClient()` | `nullptr` (base) |
| 20 | `FUN_14047cff0` | `GetOwner()` | `nullptr` (base) |
| 21 | `FUN_14047c980` | `GetWeight()` | `1.0f` (base) |
| 22–31 | stubs | — | — |
| 32 | `FUN_14047bd80` | `GetHashKey()` | `uint32_t` FNV hash |

#### Key Method Implementations

**`PropagateToChildren()` [slot 5]** — traverses the circular intrusive child list at `+0x028` and calls slot 5 on each child recursively. Used for state invalidation / reset propagation.

**`EvaluateBlend(float* outSecondary)` [slot 13]** — weighted blend tree evaluation:
```c
float weightSum = 0, blendedMain = 0;
for each child in childList:
    float childMain = child->EvaluateBlend(&childSecondary);
    float w = child->weight;                    // float stored in child link
    *outSecondary += w * childSecondary;
    if (childMain >= 1e-7f):                   // epsilon filter for inactive nodes
        weightSum   += w;
        blendedMain += w * (childMain - 1e-7f);
if (weightSum > 0.0001f):
    blendedMain /= weightSum;                  // normalize
return blendedMain;
```

**`GetHashKey()` [slot 32]** — FNV-1a variant over three 32-bit fields:
```c
return ((this->instanceHash * 0x1000193) ^ this->nameHash) * 0x1000193
       ^ this->typeHash;
// 0x1000193 is the FNV-32 prime
```

### 7.3 anmRootNode — Root of the Blend Graph

**vtable address:** `0x141d71ad8`  
**Constructor:** `FUN_140470af0`  
**Inherits from both `anmClient` (primary) and `anmNode` (embedded at +0x070)**

#### Constructor Signature

```c
anmRootNode* anmRootNode::ctor(
    anmRootNode* this,
    AnmDef*      def,       // animation definition/asset
    AnmContext*  ctx,       // context (nullable)
    uint64_t     layerId    // low 16 bits = layer index
);
```

#### Struct Layout

```c
struct anmRootNode {
// ═══ anmClient subobject @ +0x000 ══════════════════════════
    void**      clientVptr;         // +0x000 = anmClient::vftable
    // ...
    short       layerId;            // +0x00E  low 16 bits of ctor param_4
    // ...
    void*       worldCtx;           // +0x068  from g_WorldContextTable lookup
    void*       contextRef;         // +0x038  set by BindContext()

// ═══ anmNode subobject @ +0x070 ═════════════════════════════
    void**      nodeVptr;           // +0x070 = anmRootNode::vftable (as anmNode)
    // ...
    float       weight;             // +0x08C = 1.0f
    ListLink    childList;          // +0x090  sentinel (self-referencing)
    ListLink    siblingList;        // +0x0A0  sentinel
    float       field_0xB0;         // +0x0B0 = 1.0f
    float       field_0xB4;         // +0x0B4 = 1.0f
    uint16_t    childCount;         // +0x092  incremented per layer init
    uint16_t    field_0xCC;         // +0x0CC = 4
    uint32_t    field_0xD8;         // +0x0D8 = 1

// ═══ anmRootNode own fields ══════════════════════════════════
    anmRootNode* selfRef;           // +0x0E0 = this
    // ... (zeroed)                 // +0x0E8-0x11F
    ListLink    list2;              // +0x120  second self-referencing list
    uint32_t    field_0x188;        // = -1 (invalid ID sentinel)
    float       field_0x19C;        // = 1.0f
    uint64_t    field_0x1AC;        // = -1 (invalid ID sentinel)
    uint32_t    flags;              // +0x1B8  see Flags Enum below
    uint32_t    field_0x1C4;        // = 0
};
```

#### Flags Enum (field_0x1B8)

| Bit | Hex | Name | Set when |
|---|---|---|---|
| 0 | `0x000001` | `NODE_ACTIVE` | Always set on construction |
| 19 | `0x080000` | `NODE_IS_ROOT` | Always set in `anmRootNode` |
| 4 | `0x000010` | `NO_PRIMARY_ANIM` | Anim type==0 or blend weight==0 |
| 5 | `0x000020` | `NO_WORLD_CONTEXT` | No valid worldCtx found |
| 6 | `0x000040` | `HAS_ADDITIVE_LAYER` | Primary layer has additive sub-node |
| 12 | `0x001000` | `DEF_FLAG_1` | From `def->flags & 0x01` |
| 13 | `0x002000` | `DEF_FLAG_2` | From `def->flags & 0x02` |
| 15 | `0x008000` | `HAS_MULTIPLE_BONE_LAYERS` | >1 bone layer tracks in `anmMatrixArray` |
| 21 | `0x200000` | `DEF_FLAG_10` | From `def->flags & 0x10` |

#### Key Sub-functions

| Function | Address | Role |
|---|---|---|
| `anmRootNode::InitAnimLayer` | `FUN_140471510` | Factory: creates typed layer node and inserts into child list |
| `anmRootNode::BindContext` | `FUN_140689ea0` | Sets `this->contextRef = param` (trivial setter at +0x038) |
| `anmNode::CreateFromDef` | `FUN_1406f51f0` | Creates `anmMatrixArray` node for primary blend layer |
| `anmNode::ConnectSecondary` | `FUN_140479c10` | Links secondary node to primary blend node |
| `anmRootNode::SetupBlendLayer` | `FUN_140478500` | Full layer setup after primary+secondary creation |
| `anmRootNode::LookupBlendRoot` | `FUN_14047cc70` | Looks up "Blend Root" node by hash in the definition |

### 7.4 anmDataAnimator — Data Channel Base

**Constructor:** `FUN_14064ac40`

Base for all typed data-writing animator nodes. Initialized before specialization in `InitAnimLayer`.

```c
struct anmDataAnimator {
    void**        vptr;           // +0x000 = anmDataAnimator::vftable initially
    uint64_t      field_0x008;    // = 0
    uint64_t      field_0x010;    // = 0
    ListLink      listSentinel;   // +0x028  self-referencing circular list
    float         weight;         // +0x01C = 1.0f
    uint16_t      field_0x022;    // = 0
    float         weightTertiary; // +0x040 = 1.0f
    float         weightSecondary;// +0x044 = 1.0f
    float         timeScale;      // +0x048 = 1.0f
    uint32_t      flags;          // +0x018 = 0
    uint64_t      field_0x050;    // = 0
    uint16_t      listCapacity;   // +0x05C = 4
    void*         field_0x060;    // = null
    uint32_t      baseTypeId;     // +0x068 = 2
    anmRootNode*  rootNode;       // +0x070
    uint32_t      field_0x078;    // = 0
    uint16_t      layerIdx;       // +0x07A
    uint16_t      channelIndex;   // +0x07C  from AnmChannelDef
    uint32_t      field_0x080;    // from def->field_0x04 (channelCount?)
    uint16_t      typeId;         // +0x084 = 0xFFFF initially, overwritten per type
    uint16_t      field_0x086;    // from def->field_0x02
    uint16_t      field_0x088;    // from def->field_0x01
    uint32_t      field_0x08C;    // from def->field_0x08
    uint32_t      field_0x090;    // from def->field_0x0C
    uint32_t      field_0x094;    // from def->field_0x10
    uint32_t      field_0x098;    // from def->field_0x14
    uint32_t      field_0x09C;    // = 0
    void*         dataPool;       // +0x0A0  set by InitAnimLayer (slab pool ptr)
    void*         dataPtr;        // +0x0A8  = dataPool + channelIndex*8
};
```

### AnmChannelDef (definition struct, param_4 in ctor)

```c
struct AnmChannelDef {
    uint8_t  field_0x00;
    uint8_t  field_0x01;       // → anmDataAnimator::field_0x088
    uint16_t field_0x02;       // → anmDataAnimator::field_0x086
    uint16_t field_0x04;       // → anmDataAnimator::field_0x080
    uint16_t channelIndex;     // +0x06 → anmDataAnimator::channelIndex
    uint32_t field_0x08;       // → anmDataAnimator::field_0x08C
    uint32_t field_0x0C;       // → anmDataAnimator::field_0x090
    uint32_t field_0x10;       // → anmDataAnimator::field_0x094
    uint32_t field_0x14;       // → anmDataAnimator::field_0x098
    uint32_t field_0x18;       // → data pointer offset into root->field_0x30
};
```

### 7.5 anmMatrixArray — Bone Matrix Node

**Constructor:** `FUN_1406f51f0`  
**PostInit:** `FUN_1406f7970` (`PostInit_FindUIPrimarySkeleton`)  
**typeId = 0**

The central node that manages **bone transform arrays** for skeletal animation evaluation.

```c
struct anmMatrixArray : anmDataAnimator {
    // ... inherited fields ...
    uint16_t  field_0x0F0;          // = 0xFFFF (invalid index sentinel)
    void*     indexArray;           // +0x120  reverse map: slot → def position
    HashTable* boneHashTable;       // +0x128  boneId → slot (primary bones)
    HashTable* extraHashTable;      // +0x130  extraBoneId → slot (with sorted fallback)
    void*     boneHashAlloc;        // +0x138  raw allocation for boneHashTable
    void*     extraHashAlloc;       // +0x140  raw allocation for extraHashTable
    ListLink  listSentinel2;        // +0x158  second sentinel list
    uint64_t  hashKey;              // +0x170  = hash of "MS_Primary"
    void*     uiBoneBuffer;         // +0x148  allocated if uiBoneCount > 0
    uint8_t   uiBoneCount;          // +0x150  bones in "UIPrimarySkeleton"
    MatArrDef* matArrayDef;         // +0x0B0  ptr into root->field_0x30
};
```

#### Hash Table Allocation

Both hash tables use a **70% load factor**:
```
capacity = ceil(count * 1.4285715f)   // = count / 0.7
allocSize = capacity * 8 + 0x30       // header + slots
```

`FUN_1406f8b80` = `HashTable_Init`  
`FUN_140432ce0` = `HashTable_BinarySearchOrInsert`

#### UIPrimarySkeleton Discovery

`PostInit` searches for a skeleton named `"UIPrimarySkeleton"` (FNV hash computed at init) among the definition's skeleton array. If found, allocates a buffer:

```
uiBoneBuffer size = uiBoneCount * 5 * 16 bytes
```

Stride of `5 × float4 = 80 bytes` per bone — likely position + rotation quaternion + scale (4+4+4 floats = 48 bytes padded to 80), or a full 4×4 matrix plus an extra float4. This buffer is used for UI/inventory character display poses.

### 7.6 AnmAsyncResolver — Async Pose Resolver

**Constructor:** `FUN_1406fa4c0`  
**Total struct size:** ~85KB  
**RTTI:** `"AnmAsyncResolver"` @ `0x141d931c8`

Initializes three arrays of blend channels with alternating-weight sentinel pattern (`0xFFFFFFFE` = end marker):

```c
struct BlendChannel {
    float    currentValue;   // = 0.0f
    float    currentWeight;  // = 1.0f
    float    targetValue;    // = 0.0f
    float    targetWeight;   // = 1.0f
    float    value2;         // = 0.0f
    float    weight2;        // = 1.0f
    float    value3;         // = 0.0f
    float    weight3;        // = 1.0f
    float    blend[8];       // = {1.0f × 8}
    uint32_t sentinel;       // = 0xFFFFFFFE
    uint64_t reserved[2];    // = 0
};
```

Three channel pools:
- **32 channels** @ struct+0x340, stride=`0x26*8` bytes — primary channels
- **24 channels** @ struct+0x2960, stride=`0x2C*8` bytes — secondary channels
- **8 channels** @ struct+0x4A60, stride=`0x26*8` bytes — tertiary channels

Character count in `param_1[0x46]` determines how many sub-resolvers are allocated via `FUN_140721a40` (size `0x118` each).

### 7.7 Layer Type Enum

Dispatched in `anmRootNode::InitAnimLayer` (`FUN_140471510`) via `switch(*layerDef)`:

```c
enum AnmLayerType : uint8_t {
    ANM_PRIMARY_BLEND    = 0,   // "MS_Primary" blend tree root
    ANM_FLOAT            = 1,   // anmFloat / anmSimple<float, ScalerInterpolator>
    ANM_MULTISAMPLE_C2   = 2,   // anmMultiSampleTemplate<char,   2, NOINTERP>
    ANM_MULTISAMPLE_F3   = 3,   // anmMultiSampleTemplate<short,  3, NOINTERP>
    ANM_MULTISAMPLE_I4   = 4,   // anmMultiSampleTemplate<int,    4, NOINTERP>
    ANM_MULTISAMPLE_J5   = 5,   // anmMultiSampleTemplate<int64,  5, NOINTERP>
    // 6 = error/default (goto switch default)
    ANM_MULTISAMPLE_F7   = 7,   // anmMultiSampleTemplate<float,  7, ScalerInterp>
    ANM_MULTISAMPLE_H8   = 8,   // anmMultiSampleTemplate<FLOAT16,8, ScalerInterp>
    ANM_VISIBLE          = 9,   // anmVisible / anmSimple<byte, BitInterpolator>
    ANM_USER_CONST       = 10,  // anmUserConstAnimator
    ANM_AI_GRAPH         = 0xB, // anmAIGraphConnectorAnimator
    ANM_FLIPBOOK         = 0xC, // anmFlipbookAnimator
};
```

**Type 0 (PRIMARY_BLEND) special behavior:**
- Creates primary node named `"MS_Primary"` → inserts at `childList.head` (+0x098)
- Looks up `"Blend Root"` in the definition table
- Creates secondary node if "Blend Root" found → links to primary via `ConnectSecondary`
- Calls `SetupBlendLayer` for full initialization
- All other types insert at `childList.tail` (+0x0A0) — preserving evaluation order

**Type 10 (USER_CONST) extra fields:**
```c
node->field_0xB0 = 0xFFFFFFFF;                      // constID = invalid
node->constDataPtr = root->field_0x30 + def->field_0x18;  // direct data pointer
node->field_0x250 = 0;
```
Gameplay code writes values directly to `constDataPtr` to drive animation parameters from outside the graph.

### 7.8 World Context Table

```c
// Global table indexed by thread × layer
g_WorldContextTable = DAT_143b41c90;

// Per-thread context lookup:
worldCtx = g_WorldContextTable[
    threadIndex * 0x321 + (layerId & 0xFFFF)
].field_0x40;
```

This isolates animation state per-thread and per-layer, allowing concurrent evaluation of multiple entities' animation graphs. `FUN_140429670(2)` retrieves the current thread index.

---

## 8. Rendering — ChainTrail Effect

### ChainTrail_Init (`FUN_140c0abb0`)

Initializes the weapon swing trail effect. Called with a trail context struct (`pTrailCtx`).

**Phase 1: Model asset loading**
```c
wadHandle  = OpenWADPermPack(0x1A);
registry   = FindRegistryByName(wadHandle, "WAD_R_Perm", 0);
modelCount = g_ModelTable->modelCount;   // DAT_1439c7920 + 0x11e8

// Fills pTrailCtx->modelSlots[i] (stride 24 bytes each, base at pTrailCtx+0x400):
//   [+0x00] assetPtr
//   [+0x08] registryNode
```

**Phase 2: Effect type registration**  
Hashes `"ChainTrail"` and a secondary identifier string using the FNV hash, then registers/finds the effect type in `g_TrailTypeRegistry` (max 32 entries, count in `g_TrailTypeCount`).

**Phase 3: Index buffer generation** — 1280 quads (7680 indices):
```c
// Generates triangle list for quad strip:
// Each quad (4 verts) → 2 triangles → 6 indices
// Vertex layout per quad {0,1,2,3}:
//   Triangle 1: (0, 2, 3)
//   Triangle 2: (0, 1, 2)

for (int i = 0; i < 1280; i++) {
    indices[i*6+0] = base + 0;
    indices[i*6+1] = base + 2;
    indices[i*6+2] = base + 3;
    indices[i*6+3] = base + 0;
    indices[i*6+4] = base + 1;
    indices[i*6+5] = base + 2;
    base += 4;
}
GPU_UploadIndexBuffer(pTrailCtx+0x700, indices, 0x3C00, 2);  // 15360 bytes
```

**Phase 4: Material dispatch**
```c
// 16 iterations, stride 0x1A40 through DAT_146004008
for (int seg = 0; seg < 16; seg++)
    ChainTrail_SetupSegmentMaterial(&materialData[seg],
        trailCtx->modelSlots[0].registryNode,
        trailCtx->modelSlots[0].assetPtr,
        trailCtx->indexBuffer);
```

### ChainTrail_LookupModelAsset (`FUN_140c0b1c0`)

Reusable helper: formats name with `MDL_` prefix, hashes via FNV, performs registry lookup, resolves asset object via vtable call at offset `+0x58`, releases old resource.

---

## 9. Entity & Server/Client Architecture

The engine uses a **Server/Client/Context** pattern throughout all subsystems.

### Pattern

```
Server       ← owns shared data, manages clients
  └── Client ← per-entity interface, holds reference to server
        └── Context ← per-frame evaluation state

svrClientParm / svrMultiClientParm — parameter structs for client creation
```

This pattern is used by:
- `anmServer` / `anmClient` / `anmContext`
- `collision::Server` / `collision::ServerBase` / `collision::Context`
- `EntityServer` / `EntityClient` / `EntityContext`
- `goScriptServer` / `goScriptContext`

### dc:: Namespace (Data Components)

Template-based component system:
```cpp
dc::Client<T>          // e.g. dc::Client<dc::tEntity>
anmRigNode<Client<T>>  // bridges animation rig to dc component
```

Known `dc::` types from RTTI: `tEntity`, `tPoseReader`, `tMultiTrigger`, `tDriverBank`, `tDistanceNode`, `tTransformAttrNode`, `tBlendSample`, `tRBFNode`, `tTransformDriver`, `tMaterialDriver`

### Navigation

`dtTileCacheAlloc`, `dtTileCacheMeshProcess`, `dtTileCacheCompressor`, `FastLZCompressor`, `DefaultMeshProcess` — Recast/Detour navigation mesh components (open source library, PS4 port).

---

## 10. Hash & Registry Systems

### FNV-32 Variant (used everywhere) ✅

Confirmed across `ChainTrail_Init`, `anmRootNode::InitAnimLayer`, `anmMatrixArray::PostInit`, `AnmAsyncResolver::ctor`, and the `GoW2018.cs` weight normalization check:

```c
uint32_t FNVHash(const char* str) {
    uint32_t hash = 0;
    for (each char c in str) {
        uint32_t adjusted = (c < 0x20) ? (c - 0x20) & 0xFF : c;
        hash = (hash + adjusted) * 0x401 ^ ((hash + adjusted) * 0x401 >> 6);
    }
    return hash;
}
// 0x401 = 1025 = 2^10 + 1 (FNV prime variant)
// XOR with right-shift-6 is a mixing step
```

> **Note:** This is NOT standard FNV-1a. It is a custom multiplicative hash with a mixing XOR. The prime `0x401` and shift-6 are fingerprints for this specific engine.

### Global Hash Tables

| Global | Address | Description |
|---|---|---|
| `g_WorldContextTable` | `DAT_143b41c90` | Per-thread×layer world contexts |
| `g_ModelTable` | `DAT_1439c7920` | Global model table |
| `g_TrailTypeRegistry` | `DAT_14601cd80` | Registered effect types (max 32) |
| `g_TrailTypeCount` | `DAT_14601d3b4` | Count of registered effect types |
| `g_KratosNameHash` | `DAT_144ef3c20` | Pre-computed hash of `"goheroa00"` |

### Asset Registry

```
registry = FindRegistryByName(wadHandle, "WAD_R_Perm", 0)
↓
AssetRegistry_HashLookup(registry, FNVHash("MDL_" + name), localBuf, 3)
↓
returns: registryNode (lVar2), adjusts by -8 (hidden RTTI/refcount header)
↓
AssetRegistry_ResolveObject(*node->resourcePtr, 0)
↓
vtable[0x58](obj)   // AddRef / initialization
ReleaseResourceHandle(asset->field_0x30)
```

### Tagged Pointer System (in `anmMatrixArray`)

Skeleton database pointers use 2-bit tags in bits 62–63:

| Tag | Meaning |
|---|---|
| `0` | Direct pointer |
| `1` | Self-relative offset: `ptr = (value << 2 >> 2) + &field` |
| `2+` | Validated handle — calls `FUN_14196faa0(tag-2)`, crashes if invalid |

---

## 11. Memory & Allocator System

### Slab Allocator (`FUN_1403fe730` / `FUN_1403fedf0`)

Used for all animation node allocations:
```c
allocHandle = GetAllocator();             // FUN_1403fe730 — thread-local allocator
ptr = Alloc(allocHandle, size, align);   // FUN_1403fedf0
```

Common allocation sizes observed:
- `0x55B0` (21936 bytes) — async resolver sub-objects
- `0x118` (280 bytes) — resolver channel entries
- `0x10` (16 bytes) — small animation channel data

### Per-Layer Data Pool (in `anmRootNode`)

The primary layer uses a 2D pool table indexed by block:
```c
uint poolStride  = worldCtx->field_0x1C8;    // block size from world context
uint blockIdx    = dataSize / poolStride;
uint blockOffset = dataSize % poolStride;
ptr = pool_table[blockIdx][blockOffset];      // worldCtx->field_0x1E0
```

An alternative pool `FUN_140475620` exists for `anmUserConstAnimator`, accessing a root-local pool instead of the global world context pool.

### Debug Sentinel

Newly allocated data channel buffers are initialized with:
```c
*dataPtr = 0xCCCC1234;  // MSVC uninitialized memory pattern + custom marker
```

---

## 12. Known Hardcoded Character IDs

Found in `AnmAsyncResolver::ctor` (`FUN_1406fa4c0`). These are entity name strings compared via `strcmp` against the entity's display name.

| Entity ID | Character | Effect |
|---|---|---|
| `"goheroa00"` | Kratos | `g_KratosNameHash` pre-computed at startup (thread-safe, `_Init_thread_header`). Used to set `chain->clothSimMode = 8` for `"CNmimirHeadPatch1Cloth"` — special simulation mode for **Mimir's severed head** hanging from Kratos' belt. |
| `"gofreya00"` | Freya | Sets `resolver->isFreya = 1` at offset `+0x559C`. Enables character-specific resolver behavior. |
| `"gobaldur00"` | Baldur | Forces `clothSimMode = 2` on three cloth chains: `"CNbeardClothCloth"`, `"CNmustacheClothCloth"`, `"CNratTailClothCloth"` — Baldur's beard, mustache, and rat-tail hair physics. |

### Cloth Simulation Modes (`field_0x738` on chain)

| Value | Meaning |
|---|---|
| `0` | Default |
| `2` | Baldur beard/hair mode |
| `8` | Mimir head mode (special attachment to Kratos) |

---

## 13. Cross-Reference: GoW 2018 vs Ragnarök

| System | GoW 2018 (C# tool) | GoW Ragnarök (C++ tool + Ghidra) |
|---|---|---|
| **Container** | `.lodpack` per-file, text config | `.wad` multi-stream with block groups |
| **Mesh format** | `proto` + `MESH` + `MG` + `MG_gpu` | `GOWR_MESH_DEFN` + `GOWR_MESH_GPU` in WAD |
| **Skeleton** | 3×3 rotation matrix + translation | 4×4 full matrix + IBMs |
| **Bone count field** | `+0x10: uint16` | `+0x10: uint16` (identical offset ✅) |
| **Parent array** | `+0x1E + i*8: int16` | Similar offset range |
| **Joint packing** | 11-bit cross-uint32 (max 10 influences) | Same algorithm (R32_UNKNOWN mode 2) ✅ |
| **Weight packing** | 10-bit × 3 per uint32 / 1023.0 | Same R10G10B10A2 UNORM ✅ |
| **Vertex position** | float32 or R16_SNORM+AABB | Same two modes ✅ |
| **Texture swizzle** | Morton Z-curve, GCN | Morton Z-curve + RDNA (Krak/Oodle) |
| **Hash lookup** | `hash-1` fallback | Same fallback observed ✅ |
| **Animation** | Not implemented | Partial (anm graph reverse engineered) |
| **Mesh injection** | ✅ Supported (write back to .lod) | ❌ Not yet (WriteBufferToWad commented out) |

### Shared FNV Hash Implementation

Both tools use identical hash logic — confirmed by `GoW2018.cs` weight normalization check using `1023f` (= `0x3FF` decimal, matching the 10-bit unorm scale) and the engine's FNV with `0x401` prime. The GoW 2018 C# tool also computes the same style of name hash for bone lookup.

---

## 14. Symbol Rename Table

Complete list of Ghidra rename suggestions based on analysis:

### Core Functions

| Original | Renamed To | Confidence |
|---|---|---|
| `FUN_1406ee480` | `anmNode::dtor` | ✅ |
| `FUN_14047eeb0` | `anmNode::IsActive` | ✅ |
| `FUN_14047c980` | `anmNode::GetWeight` | ✅ |
| `FUN_14047bd80` | `anmNode::GetHashKey` | ✅ |
| `FUN_14047c970` | `anmNode::GetParent` | ✅ |
| `FUN_1406ee580` | `anmNode::PropagateToChildren` | ✅ |
| `FUN_1406ee970` | `anmNode::EvaluateBlend` | ✅ |
| `FUN_1406ee4e0` | `anmNode::IsReady` | ✅ |
| `FUN_14047cfd0` | `anmNode::GetServer` | ✅ |
| `FUN_14047d000` | `anmNode::GetContext` | ✅ |
| `FUN_14047cfc0` | `anmNode::GetClient` | ✅ |
| `FUN_14047cff0` | `anmNode::GetOwner` | ✅ |
| `FUN_140470af0` | `anmRootNode::ctor` | ✅ |
| `FUN_140471510` | `anmRootNode::InitAnimLayer` | ✅ |
| `FUN_140689ea0` | `anmRootNode::BindContext` | ✅ |
| `FUN_14064ac40` | `anmDataAnimator::ctor` | ✅ |
| `FUN_1406f51f0` | `anmMatrixArray::ctor` | ✅ |
| `FUN_1406f7970` | `anmMatrixArray::PostInit_FindUIPrimarySkeleton` | ✅ |
| `FUN_1406fa4c0` | `AnmAsyncResolver::ctor` | ✅ |
| `FUN_1406ead10` | `anmUserConstAnimator::Create` | ✅ |
| `FUN_1406f51f0` | `anmMatrixArray::ctor` | ✅ |
| `FUN_140479c10` | `anmNode::ConnectSecondary` | ⚠️ |
| `FUN_14047cc70` | `anmRootNode::LookupBlendRoot` | ⚠️ |
| `FUN_140478500` | `anmRootNode::SetupBlendLayer` | ⚠️ |
| `FUN_1406f51f0` | `anmNode::CreateFromDef` | ⚠️ |
| `FUN_1406f8b80` | `HashTable_Init` | ✅ |
| `FUN_140432ce0` | `HashTable_BinarySearchOrInsert` | ✅ |
| `FUN_14086a950` | `BoneKey_Normalize` | ⚠️ |
| `FUN_1406f88a0` | `SkeletonDB_FindBoneByKey` | ⚠️ |
| `FUN_140475620` | `anmRootNode::GetLocalDataPool` | ⚠️ |
| `FUN_140c0abb0` | `ChainTrail_Init` | ✅ |
| `FUN_140c0b1c0` | `ChainTrail_LookupModelAsset` | ✅ |
| `FUN_140c0a5a0` | `ChainTrail_SetupSegmentMaterial` | ⚠️ |
| `FUN_140465d40` | `GPU_UploadIndexBuffer` | ✅ |
| `FUN_140429610` | `WAD_OpenPermPack` | ✅ |
| `FUN_14054d240` | `AssetRegistry_FindByName` | ✅ |
| `FUN_140548100` | `AssetRegistry_HashLookup` | ✅ |
| `FUN_140429a20` | `AssetRegistry_ResolveObject` | ✅ |
| `FUN_140429670` | `GetVTableOrThreadIndex` | ⚠️ |
| `FUN_1406e3a70` | `ReleaseResourceHandle` | ✅ |
| `FUN_1403fe730` | `GetThreadAllocator` | ✅ |
| `FUN_1403fedf0` | `Alloc` | ✅ |
| `FUN_1403ffcb0` | `SlabAlloc` | ⚠️ |
| `FUN_1403ffee0` | `Allocator_Free` | ✅ |
| `FUN_140689d10` | `anmClient::BaseCtor` (called by anmRootNode) | ⚠️ |
| `FUN_1406e3870` | `AddRefResource` | ⚠️ |
| `FUN_141982300` | `ZeroInit` (called in anmRootNode ctor) | ⚠️ |
| `FUN_14092ab20` | `GetValidWorldContextList` | ❓ |
| `FUN_140621310` | `AnmDef_GetParent` | ❓ |
| `FUN_1406205f0` | `AnmDef_Unwrap` | ❓ |
| `FUN_14047c990` | `AnmDef_GetBlendNode` | ❓ |

### Global Data

| Original | Renamed To | Confidence |
|---|---|---|
| `DAT_143b41c90` | `g_WorldContextTable` | ✅ |
| `DAT_1439c7920` | `g_ModelTable` | ✅ |
| `DAT_14601cd80` | `g_TrailTypeRegistry` | ✅ |
| `DAT_14601d3b4` | `g_TrailTypeCount` | ✅ |
| `DAT_144ef3c20` | `g_KratosNameHash` | ✅ |
| `DAT_144ef3c28` | `g_KratosNameHash_InitFlag` | ✅ |
| `DAT_142801f38` | `g_StackCookieBase` | ✅ |
| `DAT_143ccee08` | `g_MainAllocator` | ⚠️ |

---

*Document generated from reverse engineering session. All findings are based on static analysis of the binary — no proprietary source code was accessed.*