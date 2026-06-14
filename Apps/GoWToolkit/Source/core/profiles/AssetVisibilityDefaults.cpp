#include "core/profiles/AssetVisibilityDefaults.h"
#include "Core/AssetVisibility.h"
#include "core/types/GameTypes.h"

namespace Onyx {

// Game-specific default visibility seed table. This lives in app-level code so
// the generic AssetVisibility store carries no game knowledge. Call once at
// startup, after GameTypes::RegisterGameTypes().
void RegisterGameVisibilityDefaults() {
    auto& vis = Onyx::Services::AssetVisibility::Get();

    // â”€â”€ GOW2: Structural (Internal â€” never shown) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::EntityCount,  Onyx::Services::Visibility::Internal);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::GroupStart,    Onyx::Services::Visibility::Internal);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::GroupEnd,      Onyx::Services::Visibility::Internal);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::HeaderStart,   Onyx::Services::Visibility::Internal);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::HeaderPop,     Onyx::Services::Visibility::Internal);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::Sentinel,      Onyx::Services::Visibility::Internal);

    // â”€â”€ GOW2: Hidden by default (no viewer, consumed internally) â”€â”€â”€â”€â”€
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::GfxData,       Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::PalData,       Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::Light,         Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::Collision,     Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::Script,        Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOW2, GameTypes::Flipbook,      Onyx::Services::Visibility::Hidden);

    // â”€â”€ GOWR: Structural (Internal) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::Sentinel,      Onyx::Services::Visibility::Internal);
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::ClientGuid,    Onyx::Services::Visibility::Internal);

    // â”€â”€ GOWR: Hidden by default (no viewer / internal GPU data) â”€â”€â”€â”€â”€â”€
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::MeshGpu,       Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::Model,         Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::Material,      Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::MaterialRef,   Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::AnimClip,      Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::SoundEmitter,  Onyx::Services::Visibility::Hidden);
    vis.SetDefault(Types::GameVersion::GOWR, GameTypes::LodBinding,    Onyx::Services::Visibility::Hidden);

    // Note: TextureCpu is not a TypeId â€” it's a GOWR role that maps entries
    // with GameTypes::TexturePair. The WadBrowser handles this via the role-based
    // path which now also delegates to AssetVisibility for GOWR roles.
}

} // namespace Onyx
