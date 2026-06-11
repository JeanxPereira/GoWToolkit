#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace Onyx {

enum class FieldType {
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Double,
    Vector2, // 2 floats
    Vector3, // 3 floats
    Vector4, // 4 floats
    Matrix4x4, // 16 floats
    String,  // null-terminated or fixed length
    Array,   // Array of another StructDef or basic type (requires special handling)
    HexDump, // Raw bytes
};

struct SchemaField {
    std::string name;
    FieldType type;
    size_t count = 1; // used for basic arrays, strings, or HexDump length
    
    // Construtores auxiliares
    SchemaField(std::string n, FieldType t, size_t c = 1) : name(std::move(n)), type(t), count(c) {}
};

inline size_t GetFieldTypeSize(FieldType type) {
    switch (type) {
        case FieldType::Int8:   case FieldType::UInt8:   return 1;
        case FieldType::Int16:  case FieldType::UInt16:  return 2;
        case FieldType::Int32:  case FieldType::UInt32:  return 4;
        case FieldType::Int64:  case FieldType::UInt64:  return 8;
        case FieldType::Float:  return 4;
        case FieldType::Double: return 8;
        case FieldType::Vector2: return 8;
        case FieldType::Vector3: return 12;
        case FieldType::Vector4: return 16;
        case FieldType::Matrix4x4: return 64;
        case FieldType::String:  return 1;
        case FieldType::HexDump: return 1;
        case FieldType::Array:   return 0; // Not fixed size statically unless specified by Count, handled contextually
    }
    return 0;
}

} // namespace Onyx
