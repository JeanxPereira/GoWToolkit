#include "core/formats/gowr/SmSchemaTable.h"

#include <cstring>

namespace Onyx::Gowr::SmSchema {

// Linear, deliberately. The table is a few thousand structs and the callers
// are asset loaders that touch it a handful of times per document, not per
// frame -- an index would be state to keep in sync with a generated header for
// no measurable gain.
const Struct* FindStruct(uint16_t library, uint16_t id) {
    for (const auto& s : kStructs)
        if (s.library == library && s.id == id) return &s;
    return nullptr;
}

// Only the named subset is reachable this way, and a name can be registered by
// more than one library (every animation node library re-declares
// dctools.AnimNode), so this returns the first and callers that care about the
// distinction should address the struct by (library, id) instead.
const Struct* FindStructByName(const char* name) {
    if (!name) return nullptr;
    for (const auto& s : kStructs)
        if (s.name && std::strcmp(s.name, name) == 0) return &s;
    return nullptr;
}

const Field* FindField(uint16_t library, uint16_t id, const char* name) {
    const Struct* s = FindStruct(library, id);
    if (!s || !name) return nullptr;
    for (uint16_t i = 0; i < s->fieldCount; ++i)
        if (std::strcmp(s->fields[i].name, name) == 0) return &s->fields[i];
    return nullptr;
}

} // namespace Onyx::Gowr::SmSchema
