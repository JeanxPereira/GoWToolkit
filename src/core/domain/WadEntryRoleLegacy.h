#pragma once

// Legacy classification enums used by the GOWR `WadNodeBuilder` and by the
// UI to group entries by semantic role / functional block.
//
// These will be retired in M4 once the structural rebuild lands. Until
// then they're versioned in their own header so M1's strangler-fig split
// of `WadTypes.h` doesn't drag the much larger `Entry.h` into every TU
// that only needs the enums.

// NOTE: these enums live at global scope to match the legacy layout in
// `core/WadTypes.h`. They will move into `GOW::` (and likely be retired
// entirely) in M4.

// Semantic role of a single FileDesc entry inside a Ragnarok WAD.
// Assigned by `WadNodeBuilder::Pass1_Classify()` from name patterns + size.
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
