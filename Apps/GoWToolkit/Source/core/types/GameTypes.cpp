#include "core/types/GameTypes.h"
#include "core/types/TypeCatalog.h"
#include "core/types/GameTypeTable.h"
#include <cassert>
#include <iterator>

namespace Onyx::GameTypes {

TypeId Unknown, EntityCount, GroupStart, GroupEnd, HeaderStart, HeaderPop,
    Instance, Object, Model, Mesh, Material, Texture, GfxData, PalData,
    Animation, Script, Light, Sound, Collision, Flipbook, Chunk,
    WadFile, VagAudio, VpkVideo, PssVideo, PswVideo, TextPlain,
    ShaderContainer, ShaderVertex, ShaderPixel, ShaderHull, ShaderDomain,
    ShaderCompute, ShaderLibrary, MeshGpu, MeshDefn, GameObjectProto,
    GameObjectInst, GameObjectOverride, TexturePair, MaterialRef, LodBinding,
    AnimClip, SoundEmitter, ParticleEmitter, ParticleSystem, ClientGuid,
    WadIdentity, SharedWadRef, Sentinel;

void RegisterGameTypes() {
    // Map legacy value -> the extern handle, so registration fills them in order.
    TypeId* slots[] = {
        &Unknown, &EntityCount, &GroupStart, &GroupEnd, &HeaderStart, &HeaderPop,
        &Instance, &Object, &Model, &Mesh, &Material, &Texture, &GfxData, &PalData,
        &Animation, &Script, &Light, &Sound, &Collision, &Flipbook, &Chunk,
        &WadFile, &VagAudio, &VpkVideo, &PssVideo, &PswVideo, &TextPlain,
        &ShaderContainer, &ShaderVertex, &ShaderPixel, &ShaderHull, &ShaderDomain,
        &ShaderCompute, &ShaderLibrary, &MeshGpu, &MeshDefn, &GameObjectProto,
        &GameObjectInst, &GameObjectOverride, &TexturePair, &MaterialRef, &LodBinding,
        &AnimClip, &SoundEmitter, &ParticleEmitter, &ParticleSystem, &ClientGuid,
        &WadIdentity, &SharedWadRef, &Sentinel,
    };
    static_assert(std::size(slots) == std::size(kGameTypeTable),
                  "slots[] and kGameTypeTable must stay in sync");
    auto& cat = TypeCatalog::Get();
    for (const auto& row : kGameTypeTable) {
        TypeInfo info;
        info.key = row.key; info.label = row.label; info.media = row.media;
        info.icon = row.icon;
        info.color[0]=row.color[0]; info.color[1]=row.color[1];
        info.color[2]=row.color[2]; info.color[3]=row.color[3];
        TypeId id = cat.Register(info, row.legacyValue);  // force value == legacy
        assert(row.legacyValue < std::size(slots));
        *slots[row.legacyValue] = id;
    }
}

} // namespace Onyx::GameTypes
