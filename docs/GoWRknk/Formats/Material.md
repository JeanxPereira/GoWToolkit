# Material Format Specification (GoWR PC)

## Overview
A material binds a submesh to the textures it samples, the parameters it feeds
its shader's constant buffer, and the shader permutations compiled for it.

It occupies **two** WAD entries. The `MAT_<hash>` entry holds the parameter
table. A companion entry of type 0, laid down immediately after it in
descriptor order, holds the reference list: every asset the material pulls in.
The companion carries no name of its own — the WAD names it after its first
reference, which makes it look unrelated in a browser.

Offsets below were established by decoding `r_athena00`.

## Architecture & Hierarchy

```mermaid
graph TD
    SM[Submesh] -->|index at 0x28| MAT[MAT entry]
    MAT --> Params[Parameter table]
    MAT -.paired by content.-> Refs[Companion reference list]
    Refs --> TX[Texture entries]
    Refs --> SH[Shader permutations]
    Params --> CB[Shader constant buffer]
    TX --> Pack[.texpack payload]
```

## Binary Layout

### MAT entry

| Offset | Size | Type | Name | Description |
|--------|------|------|------|-------------|
| 0x00   | 4    | u32  | Version | `0x0A` observed |
| 0x0C   | 4    | u32  | PipelineFlags | Same field the shader header carries |
| 0x10   | 8    | u64  | Hash | The material's own hash; its shaders are named after it |
| 0xC0   | 28   | u32[7] | SectionOffsets | From the start of the entry |
| 0xE0   | 12   | u16[6] | SectionCounts | Element count per section |

Section 0 is the parameter table. The remaining sections are unidentified;
their offsets repeat when a section is empty.

### Parameter entry

24 bytes each, at `SectionOffsets[0]`:

| Offset | Size | Type | Name | Description |
|--------|------|------|------|-------------|
| 0x00   | 8    | u64  | NameHash | Hashed parameter name; the string is not in the file |
| 0x08   | 8    | u64  | TypeHash | Shared by parameters of the same type |
| 0x10   | 2    | u16  | TypeCode | |
| 0x12   | 2    | u16  | CBufferOffset | Byte offset within the shader constant buffer |
| 0x14   | 4    | f32  | Value | |

`CBufferOffset` runs cumulatively across the table, so the entries describe a
packed constant buffer laid out in declaration order.

### Companion reference list

| Offset | Size | Type | Name | Description |
|--------|------|------|------|-------------|
| 0x00   | 4    | u32  | Count | Number of references |
| 0x04   | 4    | u32  | (0) | |
| 0x08   | 76×N | — | Entries | |

Each entry is 76 bytes:

| Offset | Size | Type | Name | Description |
|--------|------|------|------|-------------|
| 0x00   | 16   | u8[16] | GUID | The referenced asset's WAD GUID |
| 0x10   | 60   | char | Name | Null-terminated |

The final entry's name field is truncated where the payload ends, so a reader
that insists on a full 76-byte stride drops the last reference.

### Recognising a texture reference

A GUID is four u32 words. A texture's GUID is built as the string `TEXTURE\0`
in its first two words followed by the 8-byte texture hash:

```
54 58 45 54 00 45 52 55  <lo32> <hi32>
```

The words are stored little-endian, so the bytes on disk are **not** the ASCII
string — they only spell `TEXT` `URE\0` when each word is read big-endian.
Compare the byte pattern, not the string.

The last two words are the texture hash in the order the asset name prints it,
which is the key the `.texpack` index is searched by.

Everything else in the list is a shader permutation, named
`<materialhash>_<stage>_<permutation>`.

## Runtime Pipelines & Specifics

### Submesh to material

A submesh names its material by index, as a `u32` at submesh offset `0x28`. The
index is constant across every level of a part, which is what identifies it:
measured on `r_athena00`, all 18 parts resolve to exactly one of the three
values 0, 1, 2, matching the WAD's three MAT entries.

### Pairing a material with its companion

The companion follows the MAT in descriptor order, but a reader working from
the browser's entry tree no longer has that order. The pairing can be made by
content instead: the companion lists the material's shader permutations, and
those are named after the material's own hash. Matching on that proves the
pairing rather than assuming it.

### Texture roles

The channel a texture contributes is encoded in its name, as the suffix before
the trailing hash:

| Suffix | Role |
|--------|------|
| `_0d_`  | Diffuse |
| `_0n_`  | Normal |
| `_0ao_` | Ambient occlusion |
| `_0h_`  | Height |

A material commonly references textures belonging to other subjects — the
athena materials all share one `teethtongue` height map, and one references a
test checkerboard — so the subject in a texture name does not identify the
material's purpose.

## Unresolved

- **Parameter names.** Only hashes are stored. Ten per material on the athena
  assets, with the strings presumably in the executable.
- **Sections 1 to 5** of the MAT entry. Section 1 holds 16-byte entries whose
  first field increments by 8 and whose last is a float; the rest were empty in
  every sample.
- **Index-to-material ordering.** That `0x28` is the material index is
  established; that index 0 means the first MAT entry in descriptor order is
  the working assumption, not a proven fact.

## Related

- `Mesh.md` — the submesh field that names the material.
- `Texture.md` — decoding the referenced texture payloads.
- `Shader.md` — the permutations the reference list names.
