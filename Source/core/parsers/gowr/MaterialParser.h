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
