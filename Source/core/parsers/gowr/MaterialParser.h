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
    uint16_t typeCode      = 0;
    uint16_t cbufferOffset = 0;  // byte offset within the shader constant buffer
    float    value         = 0.0f;
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
