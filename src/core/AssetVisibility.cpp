#include "core/AssetVisibility.h"
#include "core/domain/Entry.h"
#include "core/types/TypeRegistry.h"

namespace GOW {

AssetVisibility& AssetVisibility::Get() {
    static AssetVisibility s_instance;
    return s_instance;
}

AssetVisibility::AssetVisibility() {
    RegisterDefaults();
}

void AssetVisibility::RegisterDefaults() {
    // Helper lambdas
    auto set = [this](GameVersion ver, TypeId id, Visibility vis) {
        m_defaults[MakeKey(ver, id)] = vis;
    };

    // ── GOW2: Structural (Internal — never shown) ────────────────────
    set(GameVersion::GOW2, TypeId::EntityCount,  Visibility::Internal);
    set(GameVersion::GOW2, TypeId::GroupStart,    Visibility::Internal);
    set(GameVersion::GOW2, TypeId::GroupEnd,      Visibility::Internal);
    set(GameVersion::GOW2, TypeId::HeaderStart,   Visibility::Internal);
    set(GameVersion::GOW2, TypeId::HeaderPop,     Visibility::Internal);
    set(GameVersion::GOW2, TypeId::Sentinel,      Visibility::Internal);

    // ── GOW2: Hidden by default (no viewer, consumed internally) ─────
    set(GameVersion::GOW2, TypeId::GfxData,       Visibility::Hidden);
    set(GameVersion::GOW2, TypeId::PalData,       Visibility::Hidden);
    set(GameVersion::GOW2, TypeId::Light,         Visibility::Hidden);
    set(GameVersion::GOW2, TypeId::Collision,     Visibility::Hidden);
    set(GameVersion::GOW2, TypeId::Script,        Visibility::Hidden);
    set(GameVersion::GOW2, TypeId::Flipbook,      Visibility::Hidden);

    // ── GOWR: Structural (Internal) ──────────────────────────────────
    set(GameVersion::GOWR, TypeId::Sentinel,      Visibility::Internal);
    set(GameVersion::GOWR, TypeId::ClientGuid,    Visibility::Internal);

    // ── GOWR: Hidden by default (no viewer / internal GPU data) ──────
    set(GameVersion::GOWR, TypeId::MeshGpu,       Visibility::Hidden);
    set(GameVersion::GOWR, TypeId::Model,         Visibility::Hidden);
    set(GameVersion::GOWR, TypeId::Material,      Visibility::Hidden);
    set(GameVersion::GOWR, TypeId::MaterialRef,   Visibility::Hidden);
    set(GameVersion::GOWR, TypeId::AnimClip,      Visibility::Hidden);
    set(GameVersion::GOWR, TypeId::SoundEmitter,  Visibility::Hidden);
    set(GameVersion::GOWR, TypeId::LodBinding,    Visibility::Hidden);

    // Note: TextureCpu is not a TypeId — it's a GOWR role that maps entries
    // with TypeId::TexturePair. The WadBrowser handles this via the role-based
    // path which now also delegates to AssetVisibility for GOWR roles.
}

// ── Query ─────────────────────────────────────────────────────────────────

Visibility AssetVisibility::GetDefault(GameVersion ver, TypeId id) const {
    auto it = m_defaults.find(MakeKey(ver, id));
    return (it != m_defaults.end()) ? it->second : Visibility::Visible;
}

Visibility AssetVisibility::GetCurrent(GameVersion ver, TypeId id) const {
    Visibility def = GetDefault(ver, id);
    if (def == Visibility::Internal) return Visibility::Internal;

    auto it = m_overrides.find(MakeKey(ver, id));
    if (it != m_overrides.end()) {
        return it->second ? Visibility::Visible : Visibility::Hidden;
    }
    return def;
}

bool AssetVisibility::IsVisible(GameVersion ver, TypeId id) const {
    return GetCurrent(ver, id) == Visibility::Visible;
}

bool AssetVisibility::IsVisible(const ParsedEntry& entry, GameVersion ver) const {
    return IsVisible(ver, entry.typeId);
}

// ── User overrides ────────────────────────────────────────────────────────

void AssetVisibility::SetUserOverride(GameVersion ver, TypeId id, bool visible) {
    Visibility def = GetDefault(ver, id);
    if (def == Visibility::Internal) return; // Cannot override Internal

    uint32_t key = MakeKey(ver, id);

    // If the override matches the default, remove it (no-op override)
    bool defaultVisible = (def == Visibility::Visible);
    if (visible == defaultVisible) {
        m_overrides.erase(key);
    } else {
        m_overrides[key] = visible;
    }
}

void AssetVisibility::ClearUserOverride(GameVersion ver, TypeId id) {
    m_overrides.erase(MakeKey(ver, id));
}

void AssetVisibility::ResetAllOverrides() {
    m_overrides.clear();
}

// ── FilterPanel data ──────────────────────────────────────────────────────

std::vector<AssetVisibility::TypeVisInfo> AssetVisibility::GetFilterableTypes(GameVersion ver) const {
    std::vector<TypeVisInfo> result;

    // Walk all registered handlers and include types that have a default
    // of Hidden (filterable) or Visible (togglable to hide).
    // Skip Internal types — those never appear in the UI.
    const auto& handlers = TypeRegistry::Get().AllHandlers();

    for (const auto* handler : handlers) {
        TypeId id = handler->GetId();
        Visibility def = GetDefault(ver, id);

        // Skip Internal types — not user-toggleable
        if (def == Visibility::Internal) continue;

        // Skip Unknown type
        if (id == TypeId::Unknown || id == TypeId::COUNT) continue;

        TypeVisInfo info;
        info.id               = id;
        info.name             = handler->GetName();
        info.icon             = handler->GetIcon();
        info.defaultVis       = def;
        info.currentlyVisible = (GetCurrent(ver, id) == Visibility::Visible);
        info.hasOverride      = (m_overrides.find(MakeKey(ver, id)) != m_overrides.end());

        result.push_back(info);
    }

    return result;
}

// ── Persistence ───────────────────────────────────────────────────────────

std::vector<AssetVisibility::SerializedOverride> AssetVisibility::ExportOverrides() const {
    std::vector<SerializedOverride> result;
    result.reserve(m_overrides.size());

    for (const auto& [key, vis] : m_overrides) {
        SerializedOverride so;
        // Pack: gameVersion in high byte, typeId in low byte
        uint8_t gameVer = (uint8_t)(key >> 16);
        uint8_t typeId  = (uint8_t)(key & 0xFFFF);
        so.key     = (uint16_t)(gameVer << 8) | typeId;
        so.visible = vis ? 1 : 0;
        so._pad    = 0;
        result.push_back(so);
    }
    return result;
}

void AssetVisibility::ImportOverrides(const std::vector<SerializedOverride>& data) {
    m_overrides.clear();
    for (const auto& so : data) {
        uint8_t gameVer = (uint8_t)(so.key >> 8);
        uint8_t typeId  = (uint8_t)(so.key & 0xFF);
        uint32_t key = MakeKey(static_cast<GameVersion>(gameVer),
                               static_cast<TypeId>(typeId));
        m_overrides[key] = (so.visible != 0);
    }
}

} // namespace GOW
