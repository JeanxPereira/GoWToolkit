#pragma once
#include "TypeId.h"
#include "GameVersion.h"
#include "ITypeHandler.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>

namespace GOW {

/// Central dispatcher for all asset types.
/// Handlers register themselves at static init time via REGISTER_TYPE / REGISTER_TAG macros.
/// The WAD parser calls ResolveByTag() to identify nodes — one call, zero branching.
class TypeRegistry {
public:
    /// Meyer's singleton — safe against static init order fiasco.
    static TypeRegistry& Get();

    /// Register a handler that is dispatched by magic number (first 4 bytes of payload).
    /// Used for Tag 1 (SERVER_INSTANCE) nodes in GOW1/2.
    void RegisterByMagic(GameVersion ver, std::unique_ptr<ITypeHandler> handler);

    /// Register a handler that is dispatched by WAD tag number (structural tags).
    /// Used for tags like 0 (EntityCount), 2 (GroupStart), 3 (GroupEnd), etc.
    void RegisterByTag(GameVersion ver, uint16_t tagNum, std::unique_ptr<ITypeHandler> handler);

    /// Register a handler by TypeId directly (file-level types identified by extension).
    /// These types have no magic number — they are identified at the filesystem layer
    /// (TOC/PAK entry extension). Does not enter the magic or tag maps.
    void RegisterByTypeId(std::unique_ptr<ITypeHandler> handler);

    /// The single entry point for WAD parsing.
    /// Checks tag-based map first; if the tag maps to "read magic from payload",
    /// reads the first 4 bytes and dispatches via the magic-based map.
    /// Returns nullptr if no handler is registered for this tag/magic combo.
    ITypeHandler* ResolveByTag(GameVersion ver, uint16_t tagNum,
                               const uint8_t* payload, size_t payloadSize) const;

    /// Direct lookup by TypeId (useful for UI code that already knows the type).
    ITypeHandler* Resolve(TypeId id) const;

    /// Iterate all registered handlers (useful for debug/listing).
    const std::vector<ITypeHandler*>& AllHandlers() const { return m_allHandlers; }

private:
    TypeRegistry() = default;

    // Composite key: (GameVersion << 32) | value
    static uint64_t MakeKey(GameVersion ver, uint32_t value) {
        return (static_cast<uint64_t>(ver) << 32) | value;
    }

    // Tag number that means "dispatch by magic instead"
    static constexpr uint16_t TAG_SERVER_INSTANCE = 1;

    // Owns all handlers
    std::vector<std::unique_ptr<ITypeHandler>> m_owned;

    // Non-owning index: all registered handlers
    std::vector<ITypeHandler*> m_allHandlers;

    // (version, magic) → handler — for payload-identified types
    std::unordered_map<uint64_t, ITypeHandler*> m_magicMap;

    // (version, tagNum) → handler — for structural tags
    std::unordered_map<uint64_t, ITypeHandler*> m_tagMap;

    // TypeId → handler — for direct lookup
    std::unordered_map<uint32_t, ITypeHandler*> m_idMap;
};

} // namespace GOW
