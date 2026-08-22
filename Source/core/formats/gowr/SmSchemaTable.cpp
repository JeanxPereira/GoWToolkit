#include "core/formats/gowr/SmSchemaTable.h"

#include <cstring>

namespace Onyx::Gowr::SmSchema {

// Linear, deliberately. The table is 1427 structs and the callers are asset
// loaders that touch it a handful of times per document, not per frame -- an
// index would be state to keep in sync with a generated header for no
// measurable gain.
const Struct* FindStruct(uint16_t id) {
    for (const auto& s : kStructs)
        if (s.id == id) return &s;
    return nullptr;
}

const Field* FindField(uint16_t structId, const char* name) {
    const Struct* s = FindStruct(structId);
    if (!s || !name) return nullptr;
    for (uint16_t i = 0; i < s->fieldCount; ++i)
        if (std::strcmp(s->fields[i].name, name) == 0) return &s->fields[i];
    return nullptr;
}

} // namespace Onyx::Gowr::SmSchema
