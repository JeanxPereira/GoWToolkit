#include "core/profiles/AssetVisibilityDefaults.h"
#include "Core/AssetVisibility.h"
#include "core/types/GameTypes.h"

namespace Onyx {

// Game-specific default visibility seed table. This lives in app-level code so
// the generic AssetVisibility store carries no game knowledge. Call once at
// startup, after GameTypes::RegisterGameTypes().
void RegisterGameVisibilityDefaults() {
    auto& vis = AssetVisibility::Get();

    // â”€â”€ GOW2: Structural (Internal â€” never shown) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    vis.SetDefault(GameVersion::GOW2, GameTypes::EntityCount,  Visibility::Internal);
    vis.SetDefault(GameVersion::GOW2, GameTypes::GroupStart,    Visibility::Internal);
    vis.SetDefault(GameVersion::GOW2, GameTypes::GroupEnd,      Visibility::Internal);
    vis.SetDefault(GameVersion::GOW2, GameTypes::HeaderStart,   Visibility::Internal);
    vis.SetDefault(GameVersion::GOW2, GameTypes::HeaderPop,     Visibility::Internal);
    vis.SetDefault(GameVersion::GOW2, GameTypes::Sentinel,      Visibility::Internal);

    // â”€â”€ GOW2: Hidden by default (no viewer, consumed internally) â”€â”€â”€â”€â”€
    vis.SetDefault(GameVersion::GOW2, GameTypes::GfxData,       Visibility::Hidden);
    vis.SetDefault(GameVersion::GOW2, GameTypes::PalData,       Visibility::Hidden);
    vis.SetDefault(GameVersion::GOW2, GameTypes::Light,         Visibility::Hidden);
    vis.SetDefault(GameVersion::GOW2, GameTypes::Collision,     Visibility::Hidden);
    vis.SetDefault(GameVersion::GOW2, GameTypes::Script,        Visibility::Hidden);
    vis.SetDefault(GameVersion::GOW2, GameTypes::Flipbook,      Visibility::Hidden);

    // â”€â”€ GOWR: Structural (Internal) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    vis.SetDefault(GameVersion::GOWR, GameTypes::Sentinel,      Visibility::Internal);
    vis.SetDefault(GameVersion::GOWR, GameTypes::ClientGuid,    Visibility::Internal);

    // â”€â”€ GOWR: Hidden by default (no viewer / internal GPU data) â”€â”€â”€â”€â”€â”€
    vis.SetDefault(GameVersion::GOWR, GameTypes::MeshGpu,       Visibility::Hidden);
    vis.SetDefault(GameVersion::GOWR, GameTypes::Model,         Visibility::Hidden);
    vis.SetDefault(GameVersion::GOWR, GameTypes::Material,      Visibility::Hidden);
    vis.SetDefault(GameVersion::GOWR, GameTypes::MaterialRef,   Visibility::Hidden);
    vis.SetDefault(GameVersion::GOWR, GameTypes::AnimClip,      Visibility::Hidden);
    vis.SetDefault(GameVersion::GOWR, GameTypes::SoundEmitter,  Visibility::Hidden);
    vis.SetDefault(GameVersion::GOWR, GameTypes::LodBinding,    Visibility::Hidden);

    // Note: TextureCpu is not a TypeId â€” it's a GOWR role that maps entries
    // with GameTypes::TexturePair. The WadBrowser handles this via the role-based
    // path which now also delegates to AssetVisibility for GOWR roles.
}

} // namespace Onyx
