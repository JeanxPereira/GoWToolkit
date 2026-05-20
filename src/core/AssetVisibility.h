#pragma once
#include "core/types/GameVersion.h"
#include "core/types/TypeId.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

struct ParsedEntry;

namespace GOW {

/// Controls whether a type appears in the WAD browser tree.
enum class Visibility : uint8_t {
    Visible,   // Always shown
    Hidden,    // Hidden by default, user can toggle on
    Internal   // Never shown (structural: GroupStart, EntityCount, etc.)
};

/// Centralized, data-driven asset visibility registry.
///
/// Replaces the hardcoded IsGOWRViewable() switch and provides GOW2 filtering
/// for the first time. Each (GameVersion, TypeId) pair has a default Visibility.
/// Users can override Hidden→Visible and vice-versa through the FilterPanel.
/// Overrides are persisted in the GTKC config.
class AssetVisibility {
public:
    static AssetVisibility& Get();

    /// Should this entry be rendered in the browser tree?
    bool IsVisible(GameVersion ver, TypeId id) const;

    /// Convenience overload that reads game version from the entry's context.
    /// Uses GOW2 as default — GOWR entries checked via role→TypeId mapping.
    bool IsVisible(const ParsedEntry& entry, GameVersion ver) const;

    Visibility GetDefault(GameVersion ver, TypeId id) const;
    Visibility GetCurrent(GameVersion ver, TypeId id) const;

    /// Toggle an override. Only works for Hidden types (not Internal).
    void SetUserOverride(GameVersion ver, TypeId id, bool visible);
    void ClearUserOverride(GameVersion ver, TypeId id);
    void ResetAllOverrides();

    /// Info for the FilterPanel UI.
    struct TypeVisInfo {
        TypeId      id;
        const char* name;       // from ITypeHandler::GetName()
        const char* icon;       // from ITypeHandler::GetIcon()
        Visibility  defaultVis;
        bool        hasOverride;
        bool        currentlyVisible;
    };

    /// Returns all filterable (Hidden, not Internal) types for a game version.
    std::vector<TypeVisInfo> GetFilterableTypes(GameVersion ver) const;

    // ── Persistence ────────────────────────────────────────────────────
    // Overrides are stored as a compact array of (key, visible) pairs.
    // key = (uint8_t gameVersion << 8) | uint8_t typeId
    struct SerializedOverride {
        uint16_t key;       // (gameVersion << 8) | typeId
        uint8_t  visible;   // 0 or 1
        uint8_t  _pad = 0;
    };
    static_assert(sizeof(SerializedOverride) == 4, "Must be 4 bytes for GTKC alignment");

    std::vector<SerializedOverride> ExportOverrides() const;
    void ImportOverrides(const std::vector<SerializedOverride>& data);

private:
    AssetVisibility();

    static uint32_t MakeKey(GameVersion ver, TypeId id) {
        return (static_cast<uint32_t>(ver) << 16) | static_cast<uint32_t>(id);
    }

    // Default visibility per (ver, typeId)
    std::unordered_map<uint32_t, Visibility> m_defaults;

    // User overrides: key → visible (true = force show, false = force hide)
    std::unordered_map<uint32_t, bool> m_overrides;

    void RegisterDefaults();
};

} // namespace GOW
