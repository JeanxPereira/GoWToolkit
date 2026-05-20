#pragma once

#include <string>

namespace GOW {
namespace Gowr {

// Semantic role of a single FileDesc entry inside a Ragnarok WAD.
enum class WadEntryRole {
    // Structural / Meta
    WadIdentity,        // WAD_R_*
    SharedWadRef,       // *X_R_* entries
    Sentinel,           // PopHeap, autopad

    // Shaders
    ShaderContainer,    // 0xHASH entries
    ShaderVertex,       // name_vs_FLAGS or HASH_vs_FLAGS
    ShaderPixel,        // HASH_ps_FLAGS
    ShaderHull,         // HASH_hs_FLAGS
    ShaderDomain,       // HASH_ds_FLAGS
    ShaderCompute,      // HASH_cs_FLAGS
    ShaderLibrary,      // HASH_ls_FLAGS

    // Core Assets
    AnimClip,           // ANM_*
    TextureGpu,         // TX_* with large size (GPU data)
    TextureCpu,         // TX_* with small size (CPU descriptor)
    Material,           // MAT_HASH with size > 0
    MaterialRef,        // MAT_HASH with size == 0
    LodBinding,         // N_0_M entries
    MeshGpu,            // MG_*_gpu
    MeshDefn,           // MESH_* or MG_*
    Model,              // MDL_*

    // Game Objects
    GameObjectProto,    // goProto*
    GameObjectInst,     // go*
    GameObjectOverride, // go*_overrideInst
    SceneBundle,        // Magic 0x5A7ADA7A

    // Audio
    SoundEmitter,       // SEMW_*

    // Particle FX
    ClientGuid,         // DCClientGUID
    ParticleEmitter,    // PEM_emit_*
    ParticleSystem,     // PTC_part_*

    // Synthetic (builder-generated virtual nodes)
    TexturePair,
    ShaderGroup,
    FxGroup,
    ManifestBlock,
    ShaderBlock,
    AssetBlock,
    ParticleBlock,

    Unknown
};

// Functional block that an entry belongs to.
enum class WadBlock {
    Manifest,
    Shaders,
    Assets,
    Particles,
    Unknown
};

// Parses a FileDesc name from the Ragnarök WAD.
struct WadAssetName {
    std::string prefix;     // "MG", "ANM", "PEM", "GOWR_SHADER", "" (empty = no prefix)
    std::string base;       // asset name: "heroa00", "DCClientGUID", "autopad"
    int         lod  = -1;  // LOD index (>= 0 if present), -1 = no LOD
    std::string variant;    // "gpu", "cpu", "ps", "ps_30000207", "" = no variant
    int         wadIndex = -1; // sequential FileDesc index (number after "---")

    static WadAssetName Parse(const std::string& raw);

    std::string GroupKey() const {
        return prefix.empty() ? base : (prefix + "|" + base);
    }

    std::string CanonicalName() const;

    bool IsGpuVariant()  const { return variant == "gpu"; }
    bool IsShader()      const { return variant.find("ps") != std::string::npos ||
                                        variant.find("vs") != std::string::npos; }
    bool IsInternal()    const { return prefix.empty() && lod < 0 && variant.empty(); }
};

} // namespace Gowr
} // namespace GOW
