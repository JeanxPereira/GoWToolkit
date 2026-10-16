#pragma once
#include <Onyx/Vfs/IFile.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Onyx {

// Parses a GOWR material (MAT_*) and its companion reference list.
//
// A material is stored as two WAD entries. The MAT entry holds the parameter
// table: each parameter is keyed by a name hash and carries the byte offset it
// occupies in the shader constant buffer. The companion entry - type 0, laid
// down immediately after the MAT and named after its first reference - holds
// the list of assets the material pulls in: its textures and every shader
// permutation compiled for it.
//
// See docs/GoWRknk/Formats/Material.md.

// What a texture contributes, taken from the channel suffix in its name
// (TX_<subject>_gen_0<suffix>_<hash>).
enum class TextureRole : uint8_t {
    Unknown,
    Diffuse,           // _0d_
    Normal,            // _0n_
    AmbientOcclusion,  // _0ao_
    Height,            // _0h_
    Specular,          // _0s_
    Roughness,         // _0r_
    Metallic,          // _0m_
    Emissive,          // _0e_
    Gloss,             // _0g_   roughness/gloss; the second most common role
    Scatter,           // _0sc_  subsurface scatter
    Detail,            // _0sd_  detail overlay
    Opacity,           // _opc_/_alpha_  per-texel coverage, in its own map
};

const char* TextureRoleName(TextureRole role);

struct MatParam {
    uint64_t nameHash      = 0;  // hashed parameter name; the string is not in the file
    uint64_t typeHash      = 0;  // shared by parameters of the same type
    uint16_t typeCode      = 0;  // high byte is the width in bytes, low byte the kind
    uint16_t cbufferOffset = 0;  // byte offset within the shader constant buffer
    float    value         = 0.0f;

    // The parameter's real name, recovered from the shader that consumes it --
    // empty when no shader was readable. See NameParamsFromShaderConstants.
    std::string name;

    // Width in bytes, as the parameter table itself states it: 4 for a uint or
    // float, 8 for a float2. Reading it from `typeCode` rather than mapping
    // every code to a type keeps the widths this can check open-ended.
    uint16_t Width() const { return static_cast<uint16_t>(typeCode >> 8); }
};

struct MatReference {
    uint8_t     guid[16] = {};   // the referenced asset's WAD GUID
    std::string name;
    bool        isTexture   = false;
    bool        isShader    = false;
    uint64_t    textureHash = 0; // texpack lookup key; valid when isTexture
    TextureRole role        = TextureRole::Unknown;
};

struct GOWRMaterial {
    uint64_t                  hash = 0;   // the material's own hash
    std::vector<MatParam>     params;
    std::vector<MatReference> refs;

    // First reference carrying this role, or nullptr.
    const MatReference* Texture(TextureRole role) const;

    std::vector<const MatReference*> Textures() const;
    std::vector<const MatReference*> Shaders() const;
};

// Parses the MAT entry. `refList` is the companion entry and may be null, in
// which case only the parameter table is filled.
bool GOWRMaterialParse(const std::shared_ptr<Vfs::IFile>& mat,
                       const std::shared_ptr<Vfs::IFile>& refList,
                       GOWRMaterial& out);

// One member of the shader's material constant buffer: where it sits, how wide
// it is, and what it is called.
//
// Deliberately a mirror of core/shaders/MaterialSlots.h's MaterialSlot rather
// than a reuse of it. Reading those names means a real DXIL disassembly
// through dxcompiler.dll, and the parser layer touches only leaf Onyx API --
// so the loader converts across the seam and the parser stays free of the
// shader compiler.
struct ShaderConstant {
    uint16_t    offset = 0;  // byte offset in the material constant buffer
    uint16_t    width  = 0;  // 4 for uint/float, 8 for float2, ...
    std::string name;
};

// Names every parameter from the constant sitting at the same byte offset in
// the material buffer, and reports how many were named.
//
// The MAT's parameter table and the shader's ConstBuf__materialData are two
// descriptions of ONE buffer, so the offset is a key rather than a
// coincidence, and the join recovers the strings the file does not carry:
// 8BAAC5A79D4E5FC8 is material_cst_ModifyDepthFadeStartDist.
//
// Measured on r_athena00. MAT_14EF18DD481D00DC's nine parameters sit at
// offsets 0,4,8,12,16,20,24,32,40 and its pixel shader declares members at
// exactly those nine -- including the hole at 28, which is the padding the
// float2 material_cst_ParticleSpawnReference needs to reach its 8-byte
// alignment. MAT_DEE24F0F7562F93B is the same nine. What makes this a join on
// the material rather than a fixed table is the third: its shader inserts a
// layer_1__diffuse at 20 and BOTH sides push their floats from 20/24 to 24/28
// together.
//
// -- Why all-or-nothing ------------------------------------------------------
//
// A parameter that finds no constant at its offset abandons the whole
// material, which is left entirely unnamed. The two sides describe the same
// buffer, so a partial hit does not mean "most of it worked" -- it means the
// model is wrong, and half a table of confident names is worse than none.
// The smschema tables taught this directly: joining them to the runtime
// descriptors on offset gave zero matches because the two were different
// layouts of the same type, and any partial score would have been noise.
//
// The width is checked with the offset for the same reason. It comes from the
// parameter's own typeCode, so a float2 landing on a float slot is caught
// without either side having to agree on a type spelling.
int NameParamsFromShaderConstants(GOWRMaterial& mat,
                                  const std::vector<ShaderConstant>& constants);

// Assigns every texture's role from the shader's own slot names, and reports
// how many changed. Does nothing and returns 0 unless the counts match.
//
// `slotNames` is the material cbuffer's TEXTURE slots in offset order (see
// core/shaders/MaterialSlots.h). When there are exactly as many of them as
// the material has texture references, the Nth reference fills the Nth slot
// and the shader's name for that slot IS the channel -- Wound_diffuse,
// layer_0__alpha and so on. That beats reading the file name, which has no
// tag at all for maps like TX_baldur00_damagehealing01_cut_flt.
//
// The equal-count precondition is what makes this exact rather than a guess.
// It does not always hold: Baldur's beard has eleven slots and ten textures,
// with one slot (material_firefrostemissive) left unfilled and nothing in
// the file saying which. The gap runs from 3 to 22 across his materials, so
// there is no offset to correct by -- when it does not hold, the caller
// falls back to the name.
//
// A slot naming a channel this toolkit cannot use (the region system's
// material_mudsnow*, a layer above 0) leaves its texture Unknown rather than
// forcing a role onto it.
int AssignRolesFromShaderSlots(GOWRMaterial& mat,
                               const std::vector<std::string>& slotNames);

// Re-reads every `_o_`-tagged texture in `mat` as coverage rather than
// ambient occlusion, and reports how many changed.
//
// The tag is genuinely ambiguous in the data: TX_hair_o is a gradient AO
// map, TX_baldur00_beard_o is a bimodal coverage mask, and the name cannot
// tell them apart. Only the material's own shader can -- call this when it
// declares a layer_N__alpha slot and no layer_N__ao (see
// core/shaders/MaterialSlots.h). `_ao_` is never touched: that spelling is
// unambiguous.
int ReclassifyOcclusionAsCoverage(GOWRMaterial& mat);

// Parses just a companion reference list. Exposed for the positional link:
// the caller pairs a MAT with the record that follows it and confirms the
// pairing by checking that a shader reference is named after the material.
bool GOWRMaterialParseRefs(const std::shared_ptr<Vfs::IFile>& refList,
                           std::vector<MatReference>& out);

} // namespace Onyx
