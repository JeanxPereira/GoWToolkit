#pragma once

// The channel slots a GOWR material's pixel shader declares.
//
// A material's shader carries a constant buffer named ConstBuf__materialData
// whose members name every channel that material uses:
//
//     uint layer_0__diffuse;    ; Offset:   28
//     uint layer_0__normal;     ; Offset:   32
//     uint layer_0__alpha;      ; Offset:   36
//     uint layer_1__diffuse;    ; Offset:   40
//
// That is the material's own statement of what it is, and it settles a
// question the file names cannot. GOWR's channel tags are ambiguous: `_o_`
// is ambient occlusion on TX_hair_o (a gradient) and coverage on
// TX_baldur00_beard_o (a bimodal mask), and no amount of reading the name
// tells the two apart. The shader does -- Baldur's beard material declares
// layer_0__alpha and no layer_0__ao at all, so its `_o_` texture can only be
// the alpha.
//
// -- Why the slot NAMES and not a positional map ----------------------------
//
// The tempting reading is that the Nth texture reference fills the Nth slot,
// and it nearly holds: the beard's ten references line up against the eleven
// uint slots with regionidmap on material_mudsnowregionid, dynamicmaterial_nm
// on material_mudsnownormal, and its four beard maps on the four layer_*
// slots. Nearly, because one slot (material_firefrostemissive) has no texture
// and nothing in the file says which. The gap between the parameter count and
// the texture count varies from 3 to 22 across Baldur's materials, so there is
// no offset to correct by. The slot SET is solid; the position is not.
//
// -- Cost -------------------------------------------------------------------
//
// The names live in the DXBC container's STAT chunk, inside LLVM bitcode
// whose string table is bit-packed -- scanning the bytes for them finds
// nothing, which is what the first attempt did. Reading them means a real
// disassembly through dxcompiler.dll, so this is called once per material and
// the result cached by the shader's own bytes.

#include <cstdint>
#include <string>
#include <vector>

namespace Onyx {

struct MaterialSlot {
    std::string name;    // e.g. "layer_0__alpha"
    std::string type;    // "uint", "float", "float2", ...
    uint16_t    offset;  // byte offset in the material constant buffer
};

// Reads ConstBuf__materialData out of a complete DXBC container (starting at
// its 'DXBC' magic). Returns false and fills `error` when the container has no
// such buffer or the disassembler is unavailable -- both are ordinary
// outcomes, not faults: a shader may legitimately declare no material data.
bool ReadMaterialSlots(const uint8_t* dxbc, size_t size,
                       std::vector<MaterialSlot>& out, std::string& error);

// The names of the buffer's TEXTURE slots, in offset order.
//
// A texture binding is a `uint` there -- a descriptor index the game fills
// in at draw time -- while the material's scalar parameters are floats. The
// two are interleaved in the buffer, so "the Nth member" is not "the Nth
// texture" and the filter matters.
std::vector<std::string> TextureSlotNames(const std::vector<MaterialSlot>& slots);

// True when the material declares a coverage channel (layer_N__alpha or
// layer_N__opacity) and no ambient-occlusion channel. That combination is
// what makes an `_o_` texture a mask rather than an occlusion map.
bool DeclaresCoverageWithoutOcclusion(const std::vector<MaterialSlot>& slots);

} // namespace Onyx
