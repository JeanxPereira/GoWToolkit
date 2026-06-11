#pragma once
#include "Core/Types/TypeId.h"

namespace Onyx::GameTypes {

// Named handles for the GoW asset types. Valid after RegisterGameTypes() runs
// (called once at startup, before any parse). Declared extern; defined in
// GameTypes.cpp. Names match the legacy TypeId enum values one-for-one.
extern Onyx::Types::TypeId Unknown, EntityCount, GroupStart, GroupEnd, HeaderStart, HeaderPop,
    Instance, Object, Model, Mesh, Material, Texture, GfxData, PalData,
    Animation, Script, Light, Sound, Collision, Flipbook, Chunk,
    WadFile, VagAudio, VpkVideo, PssVideo, PswVideo, TextPlain,
    ShaderContainer, ShaderVertex, ShaderPixel, ShaderHull, ShaderDomain,
    ShaderCompute, ShaderLibrary, MeshGpu, MeshDefn, GameObjectProto,
    GameObjectInst, GameObjectOverride, TexturePair, MaterialRef, LodBinding,
    AnimClip, SoundEmitter, ParticleEmitter, ParticleSystem, ClientGuid,
    WadIdentity, SharedWadRef, Sentinel;

// Registers every GoW type into the TypeCatalog, in legacy enum order so each
// handle's .value equals its old enum value (GTKC V9 persistence stability).
// Idempotent. Call once at program startup (see Main wiring).
void RegisterGameTypes();

} // namespace Onyx::GameTypes
