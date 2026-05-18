# GOWR Skeleton / Rig — Analysis & Fix Plan

> **Scope**: God of War Ragnarök **PC** (Steam/EGS build). PS4/PS5 structures may differ — endianness, alignment and bone-index packing may have variants. Marked as `TODO PS4/PS5` where applicable.
>
> **Status**: reverse analysis complete. Implementation pending.
>
> **Sources**:
> - Ghidra decompilation of `GoWR.exe` (PC) via `ghidra-mcp`
> - C# ref: `/Users/jeanxpereira/CodingProjects/GoWRknk/GoWRknk/GoWRknk.cs`
> - C++ ref: `/Users/jeanxpereira/CodingProjects/GOWTool/src/Rig.cpp`
> - Current code: `src/core/parsers/gowr/ProtoParser.cpp`, `MeshParser.cpp`, `MgParser.cpp`, `core/loaders/GOWRLoaders.cpp`, `rendering/SceneRenderer.cpp`

---

## 1. Symptom

GOWR models (skinned + rigid) render with deformed mesh — collapsed submeshes, incorrectly rotated bones, parts "exploding" from the origin. Behavior nicknamed "broken bones."

## 2. Proto file layout (goProto*)

Confirmed by triangulation between Ghidra (`FUN_1406ecc20` constructor + `FUN_1406ed6b0` memcpy), GoWRknk.cs, and GOWTool/Rig.cpp.

```
+0x00..0x10     header (16B, content unused by parser)
+0x10           uint16 boneCount   (high u16 = 0; reading as u32 also works)
+0x14           int32 unused
+0x18           parent table: N entries × 8 bytes
                  each entry: 3× int16 (skip) + int16 parent
+0x18 + 8N      padding 1: 8B × N
+0x18 + 16N     padding 2: 16B × N
+0x18 + 32N     fixed header block: int64 + 4× int32 + 64B = 88B = 0x58
+0x70 + 32N     **Table A**: N × 64B  (mat4 COLUMN-major — local transforms)
+0x70 + 96N     **Table B**: N × 64B  (mat4 COLUMN-major — purpose unclear)
```

> **Column-major confirmed** by Ghidra `FUN_140699110` matmul ordering: each
> mat4 entry stores 4 columns × 4 floats each sequentially. Linear bytes
> [0..15] = column 0, [16..31] = column 1, etc. Reading as row-major and
> transposing (older code) yields the INVERSE rotation and corrupts
> translation, producing the "spider" bone debug pattern.

### Parent table entry

```c
struct ProtoParentEntry {     // 8 bytes
    int16  _skip0;
    int16  _skip1;
    int16  _skip2;
    int16  parent;            // -1 = root
};
```

### Table A vs Table B

Determined via `FUN_1406ed6b0` (init copy) + `FUN_140469640` (anim compose):

| Table | Content | Runtime usage |
|---|---|---|
| A | Local parent→joint matrices (rest pose) | Copied to `skel[+0x90] + 0` during init |
| B | Pre-composed world rest matrices | Copied to `skel[+0x90] + N*64`; **read directly** as skinning palette base |

**Critical**: Table B is **NOT an IBM**. The runtime uses Table B directly as the skinning matrix (only with `anim_override × Table_B[i]` if an override is present). Our current interpretation (`worldMat = inverse(ibm)`) is **wrong** — do not invert.

## 3. Runtime skeleton struct layout

Extracted from `FUN_1406ecc20` (constructor) + `FUN_140737a20` (validators) + `FUN_140469640` (consumer).

```
GameObject @ +0x1b0  → Skeleton*

Skeleton struct:
  +0x40..0x60    [4× ptr]    GPU bone-buffer triple-buffer slots
  +0x58          ptr → proto data + 0x18 (parent table)
  +0x60          ptr → proto data + 32N + 32 (anim_override header)
                                 +4 = count; +8 = rel offset
  +0x70          ptr → proto data + (N-1)*8 + 0x20
  +0x78          ptr → bone-buffer reuse handle
  +0x88          ptr → animSkeleton (game object that drives this)
  +0x90          ptr → runtime mat buffer (N × 128 bytes, ALLOCATED at runtime)
  +0x98          ptr → m_boneBuffer (GPU skinning palette, ALLOCATED at runtime)
  +0xa0          ptr → bone visibility/dirty bitmask
  +0xa8          int32 boneCount (duplicate)
  +0xba          uint16 boneCount (canonical)
  +0xbc          uint16 flags (bit 0x80 = use GPU palette / animated)
  +0xbe          byte  (3 if flag 0x80, else 0)
  +0xbf          byte  atomic dirty flag
```

### Buffer @ skel[+0x90]

Size: `N × 128` bytes (allocated by `FUN_1406ed1a0`/`FUN_1406ecfa0`).

```
[0      .. N*64 ]   Copy of Table A (local matrices)
[N*64   .. N*128]   Copy of Table B (skinning base matrices)  -- read by anim compose
```

### Buffer @ skel[+0x98] (m_boneBuffer)

GPU palette. **Triple-buffered** (3 slots). Each slot = `N × 48 bytes` (mat3x4 row-major affine).

Layout per bone:
```
row 0: [m00  m01  m02  tx ]
row 1: [m10  m11  m12  ty ]
row 2: [m20  m21  m22  tz ]
```

## 4. Anim compose pipeline (FUN_140469640)

```c
skel       = gameObj[+0x1b0]
animOvr    = skel[+0x60]                          // ptr to anim override header in proto
if (animOvr[+4] == 0) animOvr = NULL              // no override
else                  animOvr += animOvr[+8]      // relative offset → anim matrices

matsBase   = N*64 + skel[+0x90]                   // = base of Table B in runtime buffer

for (i = 0; i < N; ++i) {
    if (i == 0) {
        M = identity4x4                            // root is always identity
    } else {
        M = matsBase[i]                            // mat4 from Table B
        if (animOvr) {
            anim_local = animOvr[i]                // mat4
            M = anim_local × M                     // override applied
        }
    }
    // writes M as mat3x4 (3 rows × 4 cols) into the GPU palette slot
    gpu_palette[i] = mat3x4(M)
}
```

**Notes**:
- No multiplication by IBM — vertices are already in space ready for the palette
- No explicit hierarchical compose here — composition done at another stage OR Table B is already pre-composed world-rest
- No transposition between source and dest — both row-major

## 5. Bugs in current code

### BUG #1 — Wrong palette format (mat4 + unnecessary IBM mul)

**File**: `src/rendering/SceneRenderer.cpp:403`
```cpp
m_jointPalette[i]  = j.renderMat * j.bindToJointMat;
```

**Problem**:
1. Multiplication by `bindToJointMat` (IBM) — does not happen in the game's runtime
2. Shader uniform typed as `mat4` — runtime uses `mat3x4` row-major affine

**Fix**:
```cpp
m_jointPalette[i] = j.renderMat;     // no IBM
```
Shader: keep `mat4` but with bottom row hardcoded `[0,0,0,1]`, OR switch to `mat3x4` for an exact match.

### BUG #2 — Erroneous inversion of Table B in ProtoParser

**File**: `src/core/parsers/gowr/ProtoParser.cpp:142-150`

**Current problem**:
```cpp
bool ibmIdentity = (a00 > 0.999f && a11 > 0.999f && a22 > 0.999f && ...);
glm::mat4 worldMat = ibmIdentity ? composedWorld[j] : glm::inverse(ibm);
glm::mat4 bindToJoint = ibmIdentity ? glm::inverse(composedWorld[j]) : ibm;
```

Treats Table B as an IBM and inverts it. **Wrong** — Ghidra confirms Table B is used directly.

**Fix**:
```cpp
glm::mat4 worldMat = mat_from_table_B;        // no inversion
obj->joints[j].renderMat = worldMat;
obj->joints[j].bindToJointMat = glm::mat4(1); // IBM not used in GOWR
```

### BUG #3 — MG per-submesh skin palette not read

**File**: `src/core/parsers/gowr/MgParser.cpp`

`GoWRknk.cs:298-328` shows that each submesh has a local bone palette:
```
per mg-def @ defOff:
  +0x00  uint16  parentBone
  +0x02  uint8   lodCount
  +0x38  uint32[lodCount]                       → skin list offsets
  per skin list @ skinOff:
    +0x00  uint32  skinBoneCount (N)
    +0x04  4 bytes pad
    +0x08  6 bytes pad
    +0x0E  uint16[N]  globalBoneIdx
```

Skinned vertices use LOCAL indices (0..N-1) into the submesh palette. Without the remap → wrong bones applied.

**Fix**: extend `GOWRMgParser::Parse` to return `{parentBone, skinPalette[]}` per submesh.

### BUG #4 — 11-bit packed bone-idx (modes 6 and 10 influences) not supported

**File**: `src/core/parsers/gowr/MeshParser.cpp:261-276`

Only reads `Uint8` or `Uint16` raw. The C# reference has 3 modes via `num46`:

| num46 | Influences | Bone-idx encoding |
|---|---|---|
| 1 | 4 | 4× u16 or 4× u8 raw ✓ (covered) |
| 2 | 7 | 7× u16 + 1× u16 pad — **missing** |
| 3 | 10 | 11-bit packed spanning 3+ uint32s — **missing** |

10-influence decoding (direct port from `GoWRknk.cs:707-730`):
```cpp
uint32_t u;
u = read_u32();
idx[0] = u >> 21;
idx[1] = (u >> 10) & 0x7FF;
uint32_t low10 = (u & 0x3FF) << 1;
u = read_u32();
idx[2] = (u >> 31) | low10;
idx[3] = (u >> 20) & 0x7FF;
idx[4] = (u >> 9) & 0x7FF;
uint32_t low9 = (u & 0x1FF) << 2;
u = read_u32();
idx[5] = (u >> 30) | low9;
idx[6] = (u >> 19) & 0x7FF;
idx[7] = (u >> 8) & 0x7FF;
uint32_t low8 = (u & 0xFF) << 3;
u = read_u32();
idx[8] = (u >> 29) | low8;
idx[9] = (u >> 18) & 0x7FF;
```

Without this → models with >4 weights/vert (Kratos, Atreus, main NPCs) will have broken skinning.

### BUG #5 — Rigid vs Skinned detection is external and destructive

**File**: `src/core/loaders/GOWRLoaders.cpp:311-314`
```cpp
for (auto& v : p.vertices) {
    v.boneIndices = glm::uvec4(0, 0, 0, 0);
    v.boneWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
}
```

Always forces rigid mode. Destroys legitimate skinning data.

**Fix**: `MeshParser::ReadVertices` detects the presence of semantic 9/10 → sets `MeshPart::isRigid`. GOWRLoaders only forces (1,0,0,0) if `isRigid == true`.

### BUG #6 — BuildLocalTRS uses Q.14 Euler for GOWR

**File**: `src/rendering/SceneRenderer.cpp:249-318`

Reconstructs local TRS from `vectors4/5/6` (GOW2 Q.14 format). The GOWR ProtoParser only populates `vectors4` (translation); `vectors5/6` remain zero/identity. During animation → identity rotations → mesh collapses.

**Fix**: for GOWR, animation consumes a 64B mat4 per bone (`animBoneData` format). Add a path that accepts mat4 directly, or populate `matrixes1` in `ProtoParser` and use Matrixes1 in the compose step.

### BUG #7 — Default rigid weights are zeroed out

**File**: `src/core/parsers/gowr/MeshParser.cpp:141-144`
```cpp
v.boneWeights = glm::vec4(0.0f);
v.boneIndices = glm::uvec4(0u);
```

When a submesh is rigid (no semantic 9/10), all vertices get `weight=0` → shader applies no skinning → vertex ends up in the wrong space.

**Fix**: detect `hasBoneSemantic` while reading components; if false, initialize with `weight=(1,0,0,0), idx=(0,0,0,0)` (idx 0 maps via jointMap to the parentBone).

## 6. Implementation order

By decreasing visual impact:

1. **BUG #2** (IBM inversion) — trivial fix in `ProtoParser.cpp`, corrects bone orientation in rest pose
2. **BUG #1** (palette × IBM) — remove mul in `SceneRenderer::ComputeJointPalette`
3. **BUG #5 + #7** (rigid detection + default weights) — `MeshParser::ReadVertices` + `GOWRLoaders`
4. **BUG #3** (MG skin palette) — extend `MgParser` + use in `GOWRLoaders`
5. **BUG #4** (11-bit packing) — add paths in `MeshParser::ReadVertices` for num46 == 2 / 3
6. **BUG #6** (anim format) — add GOWR path in `AnimationPlayer`/`SceneRenderer`

## 7. Ghidra reference functions (PC build)

| Symbol | Address | Function |
|---|---|---|
| `FUN_1406ecc20` | `0x1406ecc20` | Skeleton constructor (reads proto, populates struct fields) |
| `FUN_1406edd50` | `0x1406edd50` | GPU palette alloc (`m_boneBuffer`, 3× N × 48B) |
| `FUN_1406ed1a0` | `0x1406ed1a0` | Runtime buffer alloc (`skel[+0x90]`, N × 128B) |
| `FUN_1406ecfa0` | `0x1406ecfa0` | Wrapper: ed1a0 + ed6b0 |
| `FUN_1406ed6b0` | `0x1406ed6b0` | Init copy: proto Table A → skel[+0x90] |
| `FUN_140469640` | `0x140469640` | Per-frame anim compose → GPU palette write |
| `FUN_14062cc10` | `0x14062cc10` | GameObject anim orchestrator (calls ed6b0 + 469640) |
| `FUN_14061fe80` | `0x14061fe80` | GameObject ctor (creates skeleton via 6ecc20) |
| `FUN_140737a20` | `0x140737a20` | SetJointDirection (validates skel + boneCount) |

## 8. Expected differences on PS4/PS5

> **Unconfirmed** — analysis done only on the PC build. Hypotheses to investigate when a console build is available:

- **GNF/GNF2 vs DXGI**: texture/render-target packing differs, but skeleton/proto is likely identical.
- **Endianness**: PS4/PS5 are little-endian like PC, no reordering expected.
- **Bone-idx packing**: the 11-bit heuristic may have slightly different encoding (PS5 wave64 SIMD vs PC RDNA2 wave32). Check the first bytes of the skin block on a PS4 build first.
- **mat3x4 vs mat4 GPU palette**: AMD GCN/RDNA layouts may align differently. PC confirmed mat3x4 row-major.
- **animSkeleton overlay** (`skel[+0x60]` anim_override): the header format (+4 count, +8 rel offset) may change between versions.

## 9. Validation

No test suite. Manual validation via UI:
- Kratos GOWR (skinned, 10-inf) → renders in bind pose without distortion
- Atreus (skinned, 4-6 inf) → bind pose OK
- Static prop (rigid, MG single parentBone) → correct position in rest pose
- Mixed prop with cloth (hair, clothing) → submeshes aligned

Recommended diagnostic logging in `MeshParser::ReadVertices` + `GOWRLoaders::SharedGowrMeshLoad`:
```
[smIdx]: rigid=%d  num46=%d  jointMap.size=%zu  vert[0].idx=(%u,%u,%u,%u)  weight=(%.2f,%.2f,%.2f,%.2f)
```
Cross-reference with equivalent logs from GoWRknk.exe running the same file.