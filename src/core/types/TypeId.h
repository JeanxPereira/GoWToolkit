#pragma once
#include <cstdint>

namespace GOW {

/// Compile-time identity for every known asset/node type across all GOW games.
/// Canonical type identifiers for WAD assets across all game profiles.
enum class TypeId : uint32_t {
    Unknown = 0,

    // ── Structural WAD tags (identified by tag number, no payload) ──
    EntityCount,        // tag 0
    GroupStart,         // tag 2
    GroupEnd,           // tag 3
    HeaderStart,        // tag 21
    HeaderPop,          // tag 19

    // ── Content types (identified by magic from payload first 4 bytes) ──
    Instance,           // go*         — game object instance
    Object,             // obj_*       — skeleton/joints container
    Model,              // mdl_*       — mesh container (parts + groups + objects)
    Mesh,               // mesh_*      — raw GPU geometry
    Material,           // mat_*       — material definition
    Texture,            // txr_*       — texture reference
    GfxData,            // gfx_*       — raw pixel data (GS format)
    PalData,            // pal_*       — palette / CLUT data
    Animation,          // anm_*       — animation clip
    Script,             // scp_*       — game script
    Light,              // lgt_*       — light source
    Sound,              // sfx_* sbk_* — sound bank
    Collision,          //             — collision mesh
    Flipbook,           // flp_*       — particle flipbook
    Chunk,              // cxt_*       — context chunk

    // ── File-level types (identified by extension from TOC) ──
    WadFile,            // .WAD
    VagAudio,           // .VAG
    VpkVideo,           // .VPK .VP1-5
    PssVideo,           // .PSS
    PswVideo,           // .PSW
    TextPlain,          // .TXT .INI .CFG .CSV .JSON .LOG — plain-text family

    // ── GOWR-specific (future) ──
    ShaderContainer,
    ShaderVertex,
    ShaderPixel,
    ShaderHull,
    ShaderDomain,
    ShaderCompute,
    ShaderLibrary,
    MeshGpu,
    MeshDefn,
    GameObjectProto,
    GameObjectInst,
    GameObjectOverride,
    TexturePair,
    MaterialRef,
    LodBinding,
    AnimClip,
    SoundEmitter,
    ParticleEmitter,
    ParticleSystem,
    ClientGuid,
    WadIdentity,
    SharedWadRef,
    Sentinel,

    COUNT  // keep last
};

/// Returns a human-readable name for a TypeId (e.g. "Model", "Material", "Instance").
const char* TypeIdName(TypeId id);

} // namespace GOW
