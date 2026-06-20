#include "core/profiles/AssetVisibilityDefaults.h"
#include <Onyx/Services/AssetVisibility.h>
#include "core/types/GameTypes.h"

namespace Onyx {

// Game-specific default visibility seed table. This lives in app-level code so
// the generic AssetVisibility store carries no game knowledge. Call once at
// startup, after GameTypes::RegisterGameTypes().
void RegisterGameVisibilityDefaults() {
    auto& vis = Onyx::Services::AssetVisibility::Get();

    // Structural (Internal — never shown)
    vis.SetDefault(GameTypes::EntityCount, Onyx::Services::Visibility::Internal);
    vis.SetDefault(GameTypes::GroupStart,  Onyx::Services::Visibility::Internal);
    vis.SetDefault(GameTypes::GroupEnd,    Onyx::Services::Visibility::Internal);
    vis.SetDefault(GameTypes::HeaderStart, Onyx::Services::Visibility::Internal);
    vis.SetDefault(GameTypes::HeaderPop,   Onyx::Services::Visibility::Internal);
    vis.SetDefault(GameTypes::Sentinel,    Onyx::Services::Visibility::Internal);
    vis.SetDefault(GameTypes::ClientGuid,  Onyx::Services::Visibility::Internal);

    // Hidden by default (no viewer / internal data)
    vis.SetDefault(GameTypes::GfxData,      Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::PalData,      Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::Light,        Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::Collision,    Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::Script,       Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::Flipbook,     Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::MeshGpu,      Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::Model,        Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::Material,     Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::MaterialRef,  Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::AnimClip,     Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::SoundEmitter, Onyx::Services::Visibility::Hidden);
    vis.SetDefault(GameTypes::LodBinding,   Onyx::Services::Visibility::Hidden);
}

} // namespace Onyx
