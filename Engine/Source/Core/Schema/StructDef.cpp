#include "StructDef.h"

namespace Onyx::Schema {

size_t StructDef::GetFixedSize() const {
    size_t size = 0;
    for (const auto& field : m_fields) {
        size += GetFieldTypeSize(field.type) * field.count;
    }
    return size;
}

} // namespace Onyx::Schema
