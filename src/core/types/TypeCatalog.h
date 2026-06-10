#pragma once
#include "TypeId.h"
#include "core/domain/MediaKind.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace Onyx {

struct TypeInfo {
    std::string key;                      // stable id, e.g. "GOW2_MESH"
    std::string label;                    // human-readable
    MediaKind   media = MediaKind::Unknown;
    const char* icon  = nullptr;          // UTF-8 codicon; nullptr => default
    float       color[4] = {0.6f,0.6f,0.6f,1.0f};
};

// Open registry: app registers its types; engine routes by metadata only.
class TypeCatalog {
public:
    static TypeCatalog& Get();

    // Registers a type and returns its handle. If forcedValue != 0, the handle
    // takes that exact value (used to preserve legacy enum values for GTKC
    // persistence stability). Re-registering the same key returns the existing
    // handle.
    TypeId Register(const TypeInfo& info, uint32_t forcedValue = 0);

    TypeId            Find(std::string_view key) const;   // {} if absent
    const TypeInfo&   Info(TypeId id) const;              // Unknown info if absent
    MediaKind         Media(TypeId id) const { return Info(id).media; }
    const char*       Label(TypeId id) const { return Info(id).label.c_str(); }
    const char*       Icon(TypeId id) const;              // default if none
    void              Color(TypeId id, float out[4]) const;

private:
    TypeCatalog();
    std::vector<TypeInfo>                   m_infos;   // index = TypeId.value
    std::unordered_map<std::string, TypeId> m_byKey;
    TypeInfo                                m_unknown; // index 0
};

// Convenience free function preserving the old call sites' spelling.
inline MediaKind KindOf(TypeId id) { return TypeCatalog::Get().Media(id); }

} // namespace Onyx
