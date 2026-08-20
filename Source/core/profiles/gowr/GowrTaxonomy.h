#pragma once

#include <cstdint>
#include <string>

// Forward-declared only: Classify() takes AssetEntry by const-ref, so the
// full <Onyx/Domain/Entry.h> (heavier) stays out of every TU that only needs
// the enums below. See core/domain/WadEntryRoleLegacy.h for the same
// reasoning applied to the legacy WadEntryRole/WadBlock aliases.
namespace Onyx { namespace Domain { struct AssetEntry; } }

namespace Onyx {
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

// What AssetEntry::profileTag used to carry (Onyx v1.1 has no per-entry
// storage, so nothing stores this any more — see Classify() below).
struct GowrProfileTag {
    WadEntryRole role       = WadEntryRole::Unknown;
    WadBlock     block      = WadBlock::Unknown;
    WadAssetName parsedName;
};

// Classifies a single FileDesc entry (by its own name + payload size) into a
// WadEntryRole. Pure function — the single source of truth also used by
// WadNodeBuilder::Pass1_Classify. Priority-ordered pattern matching; see
// GowrTaxonomy.cpp for the rules.
//
// TX_* entries split into TextureGpu ("large" payload) / TextureCpu ("small"
// descriptor) by `size >= 1024`, matching the threshold the WAD builder has
// always used for this split.
WadEntryRole ClassifyByName(const std::string& name, uint64_t size);

// Reconstructs the tag WadNodeBuilder used to store on AssetEntry::profileTag,
// purely from the entry's own name and payload size (Onyx v1.1 removed both
// the tag field and all per-entry storage). `role` comes from ClassifyByName;
// `parsedName` from WadAssetName::Parse; `block` is left at its default
// because nothing ever read it (WadNodeBuilder.cpp wrote it but had no
// consumer for it) — do not derive it.
GowrProfileTag Classify(const Onyx::Domain::AssetEntry& entry);

} // namespace Gowr
} // namespace Onyx
