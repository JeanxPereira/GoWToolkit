# God of War Engine — MAT_ Material System
## Reverse Engineering Analysis

> **Engine:** Santa Monica Studio proprietary engine (God of War 2018 / Ragnarök)  
> **Analysis tools:** Ghidra (decompilation), binary inspection  
> **Platform:** Windows x64 executable  
> **File format version:** `0x0A` (little-endian)

---

## Table of Contents

1. [Overview](#1-overview)
2. [Naming Convention and Hash Identity](#2-naming-convention-and-hash-identity)
3. [Binary File Format](#3-binary-file-format)
4. [Material Group File (`_2` suffix)](#4-material-group-file-_2-suffix)
5. [Analyzed Functions](#5-analyzed-functions)
   - [FUN_140a441d0 — Material Name Registration](#fun_140a441d0--material-name-registration)
   - [FUN_140a442c0 — Material Name Registration (Override)](#fun_140a442c0--material-name-registration-override)
   - [FUN_140b2f150 — Material Lookup by Name](#fun_140b2f150--material-lookup-by-name)
   - [FUN_140548100 — WAD Context Hash Lookup](#fun_140548100--wad-context-hash-lookup)
   - [FUN_1406e6a60 — Get Anim Client from Shared Instance](#fun_1406e6a60--get-anim-client-from-shared-instance)
6. [Inferred Data Structures](#6-inferred-data-structures)
7. [Hash Algorithm](#7-hash-algorithm)
8. [WAD Group System](#8-wad-group-system)
9. [Runtime Resolution Pipeline](#9-runtime-resolution-pipeline)
10. [Material Variant Analysis](#10-material-variant-analysis)
11. [Key Observations](#11-key-observations)

---

## 1. Overview

The `MAT_` prefix identifies **material asset files** in the Santa Monica Studio engine. A material in this context is the full rendering description of a surface: it bundles together a shader program reference (via a 128-bit GUID), up to 10 typed shader parameters (texture references, constant vectors, and scalar values), GPU sampler descriptors, and LOD fade configuration.

Files are named after a 64-bit hash of the original material string — for example, `MAT_14EF18DD481D00DC`. The original string name is not stored inside the file; the hash itself is the canonical identifier, written at offset `0x10` in the header and mirrored in the filename.

A companion file with a `_2` suffix (e.g., `MAT_DE674F96622453EB_2`) acts as a **material group index** that lists multiple `MAT_` files belonging to the same logical material, typically different LOD levels or quality tiers.

---

## 2. Naming Convention and Hash Identity

Material names are always constructed with the `MAT_` prefix by the engine, as seen in the format strings referenced by Ghidra:

```
141dd2280  ds  "MAT_%s"
```

This string is cross-referenced by three functions:

| Address | Usage |
|---|---|
| `FUN_140a441d0:140a44279` | Registration path A |
| `FUN_140a442c0:140a44370` | Registration path B (with override flag) |
| `FUN_140b2f150:140b2f176` | Runtime lookup by name |

The resulting string `"MAT_<name>"` is then hashed using a djb2-variant algorithm (see [Section 7](#7-hash-algorithm)) to produce a 64-bit identifier used for fast lookup in WAD hash tables.

The filenames on disk — `MAT_14EF18DD481D00DC`, `MAT_DE674F96622453EB`, `MAT_DEE24F0F7562F93B` — are exactly this 64-bit hash encoded as a hexadecimal suffix.

---

## 3. Binary File Format

All `MAT_` files share a fixed layout consisting of six sections. Section boundaries are defined in an offset table at `0xC0`. The format is little-endian throughout.

### 3.1 Header (`0x00 – 0xBF`, 192 bytes)

```
Offset  Size  Field
──────────────────────────────────────────────────────────
0x00    4     version        = 0x0000000A (10)
0x04    4     padding        = 0
0x08    4     padding        = 0
0x0C    4     flags          = 0x10000007
0x10    8     self_hash      64-bit hash ≡ filename suffix
0x18    8     padding        = 0
0x20    8     group_hash     64-bit texture set group affinity
0x28    4     extra          = 0x00DE272B (observed)
0x2C    1     extra_byte     = 0x21 (33)
0x2D    ...   (zeroed to 0xBF)
```

**`self_hash`** is the djb2-variant hash of `"MAT_<name>"`. It is identical to the filename suffix and serves as the canonical runtime identifier. This is the value that `FUN_140b2f150` computes from a name string and then passes to `FUN_140548100` for lookup.

**`group_hash`** encodes the material's texture-set affinity and varies between the three observed variants, even though they share the same shader GUID. It is distinct from the shader or WAD identifiers.

**`flags = 0x10000007`** is identical across all observed samples.

### 3.2 Offset Table (`0xC0 – 0xEF`, 48 bytes)

```
Offset  Size  Field
────────────────────────────────────────────────────────────
0xC0    4     off_shader_section    always 0x00F0
0xC4    4     off_texture_table
0xC8    4     off_render_states
0xCC    4     (mirrors off_render_states — empty section A)
0xD0    4     (mirrors off_render_states — empty section B)
0xD4    4     off_trailing_data
0xD8    4     trailing_data_size    always 0x30 (48)
0xDC    4     padding               = 0
0xE0    2     num_shader_params     includes scalar terminator
0xE2    2     num_textures
0xE4    4     padding               = 0
0xE8    4     num_render_states
0xEC    4     padding               = 0
```

Offsets `0xCC` and `0xD0` always equal `0xC8`, confirming those two optional sections are unused in the observed files.

### 3.3 Shader Section (`0xF0+`)

The shader section begins with a 16-byte shader GUID, followed by `(num_shader_params − 1)` 24-byte param entries, and a single 8-byte scalar terminator.

#### 3.3.1 Shader GUID (16 bytes at `0xF0`)

```
6e 03 db 3b  d5 f5 ed d9  5f 99 cf d7  97 41 6a be
```

This GUID is **identical across all three observed MAT files**, confirming they all bind to the same shader program. The GUID references a compiled shader asset elsewhere in the WAD system.

#### 3.3.2 Param Entries (`(num_params − 1) × 24 bytes`)

Each entry describes one shader parameter binding:

```
Offset  Size  Field
──────────────────────────────────────────────────────
0x00    2     type         encoding of param kind
0x02    2     slot         cbuffer register × 4
0x04    4     float_val    float32 default/scale value
0x08    16    hash128      texture GUID or float4 constant
```

**Type encodings observed:**

| Value | Name | Meaning |
|---|---|---|
| `0x0409` | `TEX_REF` | Texture sampler; `hash128` = 128-bit texture asset GUID |
| `0x0401` | `VEC4` | Constant buffer float4; `hash128` = packed float4 value |
| `0x0841` | `FLOAT×255` | Scalar parameter; `float_val` = 255.0 as default scale |

**Slot values** are cbuffer register indices multiplied by 4, stepping through `0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x28`.

**Observed slot-to-semantic mapping (standard layout):**

| Slot | Semantic |
|---|---|
| `0x00` | albedo / base color |
| `0x04` | normal map |
| `0x08` | roughness / metallic |
| `0x0C` | emissive |
| `0x10` | AO / cavity |
| `0x14` | vec4 param 0 (or extra texture in hi-res variants) |
| `0x18` | vec4 param 1 |
| `0x1C` | vec4 param 2 |
| `0x20` | scalar param A (×255) |
| `0x28` | scalar terminator |

#### 3.3.3 Scalar Terminator (8 bytes)

The last entry is always 8 bytes (no `hash128` field):

```
type=0x0401  slot=0x28  float_val=255.0
```

This entry does not carry a texture reference or constant hash. Its purpose appears to be a final multiplier or sentinel marking the end of the parameter block. `num_shader_params` in the offset table counts this entry.

**Total shader section size formula:**

```
size = 16 (GUID) + (num_params − 1) × 24 + 8
```

Verified: `14EF` → `16 + 8×24 + 8 = 216`; `DE674F` → `16 + 9×24 + 8 = 240`. Both match their respective `off_texture_table − off_shader_section` deltas.

### 3.4 Texture Slot Table (`N × 16 bytes`)

Maps shader param slots to GPU texture unit bindings:

```
Offset  Size  Field
────────────────────────────────────────────────
0x00    1     par_idx     encoded as 1 + 8 × texture_index
0x01    1     = 0x00
0x02    1     = 0x00
0x03    1     = 0xFF  (sentinel marker)
0x04    4     val1        cbuffer param ID (0x0C, 0, 1, 5, 6…)
0x08    4     tex_unit    GPU sampler slot (0–5)
0x0C    4     weight      float32; 1.0 = normal, 8.0 = LOD variant
```

The `par_idx` encoding `1 + 8 × n` encodes the param entry index `n` in the lower bits with bit 0 always set as a valid-flag. The sequential `tex_unit` (0, 1, 2, 3, 4…) directly maps to GPU texture unit bindings.

A `weight` of **8.0** appears on a 6th texture slot in `MAT_DE674F96622453EB`, suggesting a blend intensity or tiling multiplier for a secondary/detail layer.

### 3.5 Render States (`M × 16 bytes`)

Each entry describes a sampler state binding:

```
Offset  Size  Field
────────────────────────────────────────────────────────────
0x00    4     state_key
              bits [7:0]   filter mode (0x07=bilinear, 0x17=anisotropic)
              bits [15:8]  secondary filter / aniso level tier
              bits [23:16] UV set index  (0 = UV0, 1 = UV1)
              bits [31:24] material type flags (0x10 constant)
0x04    4     sampler_slot = 14 (0x0E) in all observed samples
0x08    4     binding_flags = 0x00010054 in all observed samples
0x0C    4     padding = 0
```

**Observed state keys:**

| Key | Filter | UV set |
|---|---|---|
| `0x10000207` | Bilinear | UV0 |
| `0x10000217` | Anisotropic | UV0 |
| `0x10001207` | Bilinear (alt tier) | UV0 |
| `0x10001217` | Anisotropic (alt tier) | UV0 |
| `0x10010207` | Bilinear | UV1 |
| `0x10011207` | Bilinear (alt tier) | UV1 |

The byte at `key[15:8]` differentiates between two sampler configurations at the same filter level — likely different anisotropy levels (e.g., 4× vs 8×) or different mip bias settings.

### 3.6 Trailing Data (48 bytes, fixed size)

Always 48 bytes (`0x30`), mostly zeroed. Two float32 values are consistently present:

```
+0x14  float32 = 25.0   (fade_start distance)
+0x18  float32 = 25.0   (fade_end distance)
```

These values are identical across all three observed files. They likely define the LOD fade-in and fade-out distances in game units for the material's visibility calculation.

---

## 4. Material Group File (`_2` suffix)

A companion file suffixed `_2` functions as a **material group index**. It contains a flat array of entries, each referencing one `MAT_` file belonging to the group:

```
Offset  Size  Field
──────────────────────────────────────────
0x00    8     entry_count   uint64
── per entry (72 bytes each) ──────────
0x00    16    uuid          128-bit identifier
0x10    56    name          null-terminated, padded: "MAT_<hash>"
```

**Example `MAT_DE674F96622453EB_2` (232 bytes, 3 entries):**

| Entry | UUID (128-bit) | Name |
|---|---|---|
| 0 | `59895013f2e360d4dbab05b908e86f56` | `MAT_DE674F96622453EB` |
| 1 | `00000000...08f1fd47...` | `MAT_DEE24F0F7562F93B` |
| 2 | `...55fe3422dfd6c83e...` | `MAT_14EF18DD481D00DC` |

The file groups the three MAT_ variants together, likely as quality tiers or LOD levels of the same logical material.

---

## 5. Analyzed Functions

### `FUN_140a441d0` — Material Name Registration

```c
void FUN_140a441d0(longlong param_1, char *param_2, longlong param_3, byte param_4)
```

**Role:** Registers a material name string into a global material state slot and sets up a `MAT_<name>` identifier.

**Parameters:**

| Param | Type | Role |
|---|---|---|
| `param_1` | `longlong` | Material handle or context; non-zero contributes bit 3 to flags |
| `param_2` | `char*` | Material name string (raw, without `MAT_` prefix) |
| `param_3` | `longlong` | Pointer to name suffix; if non-zero, formats `MAT_%s` into `DAT_145db03d8` |
| `param_4` | `byte` | Attribute bits; lower 3 bits packed into the global flags byte |

**Behavior:**

1. Guards against empty or null `param_2`.
2. Uses `_stricmp` to compare `param_2` against the currently active material name (`DAT_145db03a0`). If different:
   - Copies the new name into the global slot via `strncpy(..., 0x37)` (55-byte limit).
   - Zeroes `DAT_145db03d7` (likely a dirty/loaded flag).
   - Sets `DAT_145d9e6c3 = 1` (a change notification flag).
   - Computes `DAT_145db0410` flags: `(-(param_1 != 0) & 8 | param_4 & 7) * 2 | existing & 0xE1`.
   - Calls `FUN_1405f52e0()` to check for an existing resource, and if found calls `FUN_1405f52c0()` to release it.
3. If the active name slot is non-empty:
   - If `param_3 == 0`: clears the MAT identifier slot (`DAT_145db03d8 = 0`).
   - Else: formats `"MAT_%s"` with `param_3` into `DAT_145db03d8` via `FUN_140532160` (a safe `sprintf` wrapper).
   - Sets bit 0 of `DAT_145db0410` (marks material as active/loaded).

**Key globals written:**

| Global | Role |
|---|---|
| `DAT_145db03a0` | Active material name string (55-byte buffer) |
| `DAT_145db03d7` | Dirty flag |
| `DAT_145db03d8` | Formatted `MAT_<name>` identifier string |
| `DAT_145db0410` | Material flags byte (bits: loaded, priority, type) |
| `DAT_145d9e6c3` | Change notification flag |

---

### `FUN_140a442c0` — Material Name Registration (Override)

```c
void FUN_140a442c0(longlong param_1, char *param_2, longlong param_3, byte param_4)
```

**Role:** Identical to `FUN_140a441d0` with one additional step at entry: sets `DAT_143ace568 = 1` before any other logic.

**Difference from `FUN_140a441d0`:**

```c
// First line of FUN_140a442c0, absent in FUN_140a441d0:
DAT_143ace568 = 1;
```

`DAT_143ace568` appears to be a global override or priority flag — setting it before name registration suggests this path forces the material to be treated as an authoritative or explicit override, bypassing any "already registered" early-outs downstream. The two registration paths likely correspond to:

- `FUN_140a441d0`: passive/automatic registration (e.g., from a data table or streaming system).
- `FUN_140a442c0`: explicit/scripted registration (e.g., from a cutscene or material override command).

---

### `FUN_140b2f150` — Material Lookup by Name

```c
longlong FUN_140b2f150(undefined8 param_1, undefined8 param_2, longlong param_3)
```

**Role:** Given a WAD context and a material name, constructs the `MAT_<name>` string, hashes it, looks it up in the WAD system, and returns the associated `AnimClient` pointer.

**Parameters:**

| Param | Role |
|---|---|
| `param_1` | Error logging context (passed to `FUN_14150d6f0` on failure) |
| `param_2` | Material name string (suffix after `MAT_`) |
| `param_3` | `WadContext*` — the WAD context to search |

**Step-by-step behavior:**

#### Step 1 — Build the name string

```c
snprintf(&local_60, 0x38, "MAT_%s", param_2);
```

Constructs `"MAT_<name>"` into a 56-byte stack buffer (`local_60`).

#### Step 2 — Hash the name

```c
uVar8 = (bVar4 + uVar8) * 0x401;
uVar8 = uVar8 ^ uVar8 >> 6;
```

Iterates over every byte of the string, applying a djb2-variant hash (see [Section 7](#7-hash-algorithm)). The result `uVar8` is a 64-bit hash.

#### Step 3 — WAD lookup

```c
lVar1 = FUN_140548100(param_3, uVar8, local_68, 3);
```

Calls the hierarchical WAD lookup (see next function). Flags = `3` = `0b11` = search both sub-WADs (`flag&2`) and the global runtime WAD (`flag&1`).

**Three error paths** are guarded with logged messages:

| Condition | Logged message |
|---|---|
| `lVar1 == 0` | `"Could not find material %s in wad context %s, are you sure the name and level are correct?"` |
| `*(lVar1 + 0x20) == 0` | `"Material %s does not have a shared instance, are you sure this is a global animation?"` |
| `FUN_1406e6a60(shared) == 0` | `"Shared material %s does not have an anim client, are you sure this material is animated?"` |

#### Step 4 — Resolve shared instance and anim client

```c
lVar1 = *(longlong *)(lVar1 + 0x20);   // SharedInstance*
lVar2 = FUN_1406e6a60(lVar1);           // AnimClient*
```

The material node at `lVar1` carries a pointer to its `SharedInstance` at offset `+0x20`. The anim client is then retrieved from the shared instance.

#### Step 5 — Register with WAD context

```c
lVar3 = FUN_14092a310(param_3);
if (lVar3 != 0) {
    FUN_140c2aac0(lVar3, lVar1);
}
```

Registers the resolved material node with the WAD context, presumably for lifetime tracking or update callbacks.

**Return value:** `AnimClient*` on success, `0` on any error path.

---

### `FUN_140548100` — WAD Context Hash Lookup

```c
undefined8 FUN_140548100(longlong param_1, undefined8 param_2, longlong *param_3, uint param_4)
```

**Role:** Hierarchical hash-based lookup of a material (or any asset) in a WAD context tree. Returns the material node pointer in `*param_3`.

**Parameters:**

| Param | Role |
|---|---|
| `param_1` | `WadContext*` — the context node to search |
| `param_2` | 64-bit hash to look up |
| `param_3` | Output: pointer to the found asset node |
| `param_4` | Search flags (`bit 0` = search global runtime, `bit 1` = search sub-WADs) |

**Behavior:**

```c
uVar1 = FUN_1404274d0(*(undefined8 *)(param_1 + 0x78));  // hash table lookup
if (**(int **)(param_1 + 0x78) == 0) {
    *param_3 = param_1;   // match found at this node
}
```

`FUN_1404274d0` performs the actual hash comparison against the hash table stored at `param_1 + 0x78`. A result of zero at `**(param_1 + 0x78)` indicates a successful match.

**If no direct match and `flag & 2` is set:** recursively searches up to 64 child `WadContext` nodes stored at `param_1 + 0x88`:

```c
for (i = 0; i < 0x40; i++) {
    if (children[i] == 0) break;
    uVar1 = FUN_140548100(children[i], param_2, param_3, param_4 & ~2);
    if (*param_3 != 0) return uVar1;
}
```

Note that `flag & 2` is cleared when passing to children (`param_4 & 0xFFFFFFFE`), preventing infinite recursion across levels.

**If still not found and `flag & 1` is set:** searches the **global WAD runtime**:

```c
piVar2 = (int *)FUN_140427830();       // get global WAD runtime
uVar1 = FUN_1404274d0(piVar2, param_2);
if (*piVar2 == 0) {
    uVar3 = FUN_140429670(0x1a);       // allocate runtime slot (type 0x1A)
    lVar4 = FUN_14054d350(uVar3, uVar1, 1);
    *param_3 = lVar4;
    return uVar1;
}
```

`FUN_140429670(0x1a)` allocates a typed runtime object (type `0x1A` = material runtime entry). `FUN_14054d350` creates or retrieves a cached entry for the hash.

**WadContext structure inferred from access patterns:**

```c
struct WadContext {
    // ...
    /* +0x50 */  longlong  name_ptr;        // ptr to ptr to WAD name string
    // ...
    /* +0x78 */  HashTable *hash_table;     // open-address hash table of assets
    /* +0x88 */  WadContext *children[64];  // sub-WAD contexts (null-terminated array)
    // ...
};
```

---

### `FUN_1406e6a60` — Get Anim Client from Shared Instance

```c
undefined8 FUN_1406e6a60(longlong param_1)
```

**Role:** Retrieves the `AnimClient*` from a `SharedMaterialInstance`, preferring an explicit `AnimController` override if present.

**Parameter:** `param_1` = `SharedMaterialInstance*` (= `material_node + 0x20`).

**Behavior:**

```c
if (*(longlong *)(param_1 + 0x28) != 0) {
    uVar1 = FUN_1406212c0(*(longlong *)(param_1 + 0x28));
    return uVar1;
}
return *(undefined8 *)(param_1 + 0x90);
```

Two resolution paths:

1. **Explicit controller path (`+0x28` non-null):** Calls `FUN_1406212c0` on the `AnimController*` at offset `+0x28`. This function likely extracts the current animation state or wraps the controller in a client interface.

2. **Default path (`+0x90`):** Returns the `AnimClient*` stored directly at offset `+0x90`. This is the baked default animation client assigned at load time.

**`SharedMaterialInstance` structure inferred:**

```c
struct SharedMaterialInstance {
    // ...
    /* +0x28 */  AnimController *anim_controller;  // explicit override; null if default
    // ...
    /* +0x90 */  AnimClient     *anim_client;       // default baked anim client
};
```

---

## 6. Inferred Data Structures

### `WadContext`

```c
struct WadContext {
    // (unknown fields 0x00–0x4F)
    /* +0x50 */  longlong        name_offset;       // *(name_offset + 0x50) = name string
    // (unknown fields 0x58–0x77)
    /* +0x78 */  HashTable      *hash_table;         // asset lookup by 64-bit hash
    // (unknown 0x80–0x87)
    /* +0x88 */  WadContext     *children[64];       // sub-context array, null-terminated
};
```

### `MaterialNode` (asset node result from hash lookup)

```c
struct MaterialNode {
    // (unknown fields 0x00–0x1F)
    /* +0x20 */  SharedMaterialInstance *shared_instance;
};
```

### `SharedMaterialInstance`

```c
struct SharedMaterialInstance {
    // (unknown fields 0x00–0x27)
    /* +0x28 */  AnimController *anim_controller;   // null = use default
    // (unknown fields 0x30–0x8F)
    /* +0x90 */  AnimClient     *anim_client;        // default animation client
};
```

### `MaterialNode` global state (registration globals)

```c
// Global material state slot (written by FUN_140a441d0 / FUN_140a442c0)
char    DAT_145db03a0[56];    // active material name (55 chars + null)
char    DAT_145db03d7;        // dirty / loaded flag
char    DAT_145db03d8[...];   // formatted "MAT_<name>" identifier string
byte    DAT_145db0410;        // flags: bit0=active, bits1-3=type, bit3=has_handle
byte    DAT_145d9e6c3;        // change notification flag
byte    DAT_143ace568;        // override / priority flag (set by path B only)
```

---

## 7. Hash Algorithm

`FUN_140b2f150` contains an inline hash loop applied to the `"MAT_<name>"` string before passing it to the WAD lookup:

```c
uVar8 = 0;
// iterate over every byte of the string:
bVar4 = bVar5 - 0x20;
if (0x19 < (byte)(bVar5 + 0x9f)) {
    bVar4 = bVar5;   // pass-through for non-alpha bytes
}
uVar8 = (bVar4 + uVar8) * 0x401;
uVar8 = uVar8 ^ uVar8 >> 6;
```

**Pseudocode:**

```python
def hash_mat(s: str) -> int:
    h = 0
    for c in s.encode():
        # case-fold: subtract 0x20 only if byte is in [0x61..0x7A] (a-z)
        b = c - 0x20 if (c + 0x9F) & 0xFF > 0x19 else c
        h = (b + h) * 0x401
        h ^= h >> 6
        h &= 0xFFFFFFFFFFFFFFFF  # 64-bit
    return h
```

This is a **djb2-variant** with:
- A case-folding step (converts lowercase to uppercase before hashing).
- Multiplicative factor `0x401` (`= 1 + 1024 = 1 + 2^10`), a fast multiply-add via shift.
- An XOR-shift diffusion step `h ^= h >> 6`.

The resulting 64-bit value is what appears as the `MAT_` filename suffix. The filenames on disk — `14EF18DD481D00DC`, `DE674F96622453EB`, `DEE24F0F7562F93B` — are exactly the `snprintf`-formatted lowercase hex of this hash.

---

## 8. WAD Group System

Each 128-bit texture hash in a `MAT_` param entry encodes two pieces of information:

```
[bytes 0–7]   texture-specific ID    (unique per texture)
[bytes 8–15]  WAD group ID           (shared by all textures in the same WAD file)
```

The **upper 8 bytes** of the texture hash (`hash128[8:16]`) identify the WAD group — effectively the WAD archive file containing the texture. All textures from the same group are co-located in the same streaming unit.

**WAD groups observed across the three MAT_ files:**

| Group ID | Textures |
|---|---|
| `5f12e20da8559c06` | albedo, normal, roughness/metallic, emissive (params 0–3, all 3 MATs) |
| `211a6814e42a7db5` | AO/cavity (params 4+, `14EF` and `DEE24F`), VEC4 params, FLOAT param |
| `31f01aae8554ac06` | AO/cavity for `DE674F` only — a distinct WAD group |

**Notable finding:** Params 0–3 share not only the same WAD group but also **identical texture IDs** across all three files — the base PBR textures (albedo, normal, roughness, emissive) are the same asset. The divergence occurs at the AO slot and beyond.

---

## 9. Runtime Resolution Pipeline

The full chain from a material name string to a usable `AnimClient`:

```
1. caller provides name string "X"
        │
        ▼
2. FUN_140a441d0 / FUN_140a442c0
   ├─ _stricmp against active slot
   ├─ strncpy name into DAT_145db03a0
   └─ sprintf "MAT_%s" → DAT_145db03d8
        │
        ▼
3. FUN_140b2f150(log_ctx, name, wad_ctx)
   ├─ snprintf "MAT_" + name → stack buffer
   ├─ djb2-variant hash → uint64
   ├─ FUN_140548100(wad_ctx, hash, &result, 3)
   │      ├─ flag&2: search children[0..63] recursively
   │      ├─ flag&1: search global WAD runtime
   │      └─ FUN_1404274d0: hash table probe at +0x78
   ├─ check result != null   (error: "not found")
   ├─ dereference result+0x20 → SharedInstance
   ├─ check SharedInstance != null  (error: "no shared instance")
   ├─ FUN_1406e6a60(SharedInstance)
   │      ├─ if +0x28 != null → FUN_1406212c0(AnimController)
   │      └─ else → return +0x90 (default AnimClient)
   ├─ check AnimClient != null  (error: "no anim client")
   └─ FUN_140c2aac0(wad_ctx, material_node)  ← register
        │
        ▼
4. return AnimClient*
```

---

## 10. Material Variant Analysis

The three `MAT_` files analyzed represent variants of the same material, confirmed by shared shader GUID, shared base textures, and their grouping in the `_2` index file.

### `MAT_14EF18DD481D00DC` — Base variant

- **640 bytes**, 9 params, 5 textures, 3 render states
- AO from **WAD-B** (`211a6814…`)
- Mix of **bilinear + anisotropic** filtering on UV0
- No secondary texture layer

### `MAT_DE674F96622453EB` — High-quality variant

- **688 bytes**, 10 params, 6 textures, 4 render states
- AO from **WAD-C** (`31f01aae…`) — a distinct, likely higher-resolution or baked-GI AO texture
- Adds a **6th texture slot** at param slot `0x14` using the WAD-B AO as a secondary layer with **weight = 8.0** (blend multiplier or tiling scale)
- **All UV0 render states are anisotropic** — indicates this is the highest quality / closest LOD level
- Has 2 render state pairs for UV0 (different anisotropy tiers) and 2 for UV1

### `MAT_DEE24F0F7562F93B` — Extended render state variant

- **688 bytes**, 9 params, 5 textures, 6 render states
- Same textures as `14EF` (WAD-B AO, same params)
- **6 render states** — most of any variant, covering bilinear and anisotropic for both UV0 and UV1, with multiple tier entries
- Likely serves a transitional LOD role or a surface that needs both UV channels fully configured

---

## 11. Key Observations

1. **Filename = hash.** The `MAT_` filename suffix is the djb2-variant hash of `"MAT_<name>"`, computed inline in `FUN_140b2f150`. Original string names are not stored in the file.

2. **All variants share one shader.** The shader GUID `6e03db3bd5f5edd95f99cfd797416abe` is identical across all observed files, meaning a single compiled shader program handles all variants. Differentiation is achieved purely through param/texture configuration.

3. **Base textures are shared.** Params 0–3 (albedo, normal, roughness/metal, emissive) carry identical 128-bit hashes across all three files — no per-variant base texture assets.

4. **Quality is encoded in AO.** The per-variant divergence starts at param slot `0x10` (AO/cavity), which changes WAD group between the base and hi-res variants. `DE674F` also layers a secondary AO for blend-based ambient occlusion.

5. **weight=8.0 is a material shader parameter.** The non-standard weight on the 6th texture slot of `DE674F` is passed directly into the texture slot table and likely feeds a tiling multiplier or blend weight uniform in the shader.

6. **WAD lookup is hierarchical.** `FUN_140548100` searches up to 64 child `WadContext` nodes before falling back to the global runtime WAD, with each level of recursion clearing the `flag&2` bit to avoid re-traversal.

7. **Animated materials require a shared instance.** `FUN_140b2f150` explicitly guards for `*(material_node + 0x20) == 0` with the message `"are you sure this is a global animation?"`, confirming that only materials flagged as global (shared across instances) support the animation client pipeline.

8. **Two registration paths.** `FUN_140a441d0` (passive) and `FUN_140a442c0` (override via `DAT_143ace568 = 1`) provide two tiers of material name registration, likely corresponding to automatic streaming and explicit scripted overrides respectively.

---

*Analysis based on Ghidra decompilation of the God of War: Ragnarök Windows executable and binary inspection of four extracted MAT_ asset files.*