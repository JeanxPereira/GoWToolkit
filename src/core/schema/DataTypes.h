#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace GOW {

// ── Data Types ─────────────────────────────────────────────────────────────
// All supported binary field types for GOW format reflection.
enum class DataType : uint8_t {
    // Primitives
    Bool,
    Int8, UInt8,
    Int16, UInt16,
    Int32, UInt32,
    Int64, UInt64,
    Float, Double,
    Enum,

    // Vectors
    Vec2,       // 2 floats
    Vec3,       // 3 floats
    Vec4,       // 4 floats / color
    Mat4,       // 16 floats

    // Dynamic
    String,     // fixed-length char[]
    CharPtr,    // null-terminated, offset-based
    Key,        // asset reference hash
    Asset,      // asset path reference

    // Containers
    Array,      // [count][elements...]
    Struct,     // nested inline struct

    // Display-only
    HexDump,    // raw bytes
    Pad,        // skip N bytes (hidden)
};

// ── Display Hints ──────────────────────────────────────────────────────────
// Controls how a field is rendered in ImGui.
enum class DisplayHint : uint8_t {
    Default,
    Hex,            // show integers as 0xDEADBEEF
    Color,          // vec3/vec4 → color square
    Angle,          // float → degrees
    Percent,        // float 0..1 → bar
    Bitfield,       // u32 → named bits
    Hidden,         // don't render in inspector
};

// ── Enum Map ───────────────────────────────────────────────────────────────
// Maps integer values to human-readable names.
using EnumMap = std::vector<std::pair<uint32_t, const char*>>;

// ── Field Definition ───────────────────────────────────────────────────────
// Immutable descriptor for one field in a struct.
struct FieldDef {
    const char*     name;
    DataType        type;
    uint32_t        offset;         // byte offset in struct
    uint32_t        size;           // byte size (for String, HexDump, Pad)
    DisplayHint     display    = DisplayHint::Default;
    const char*     tooltip    = nullptr;
    const char*     enumName   = nullptr;   // for DataType::Enum (named lookup)
    const char*     structName = nullptr;   // for DataType::Struct/ArrayStruct
    const EnumMap*  enumMap    = nullptr;   // for DataType::Enum (inline values)
    uint32_t        countOffset = 4;        // for DataType::Array (offset to count field)
};

// ── Struct Definition ──────────────────────────────────────────────────────
// Immutable descriptor for a complete binary struct.
struct StructDefinition {
    const char*             name;
    uint32_t                size;    // total byte size
    std::vector<FieldDef>   fields;  // definition order (blob follows this order)
};

// ── Helper: get primitive byte size ────────────────────────────────────────
inline uint32_t DataTypeSize(DataType t) {
    switch (t) {
        case DataType::Bool:                          return 1;
        case DataType::Int8:   case DataType::UInt8:  return 1;
        case DataType::Int16:  case DataType::UInt16: return 2;
        case DataType::Int32:  case DataType::UInt32: return 4;
        case DataType::Enum:                          return 4;
        case DataType::Int64:  case DataType::UInt64: return 8;
        case DataType::Float:                         return 4;
        case DataType::Double:                        return 8;
        case DataType::Vec2:                          return 8;
        case DataType::Vec3:                          return 12;
        case DataType::Vec4:                          return 16;
        case DataType::Mat4:                          return 64;
        case DataType::Key:    case DataType::Asset:  return 4;
        default:                                      return 0;
    }
}

// ── Helper: human name for DataType ────────────────────────────────────────
inline const char* DataTypeName(DataType t) {
    switch (t) {
        case DataType::Bool:    return "bool";
        case DataType::Int8:    return "int8";
        case DataType::UInt8:   return "uint8";
        case DataType::Int16:   return "int16";
        case DataType::UInt16:  return "uint16";
        case DataType::Int32:   return "int32";
        case DataType::UInt32:  return "uint32";
        case DataType::Int64:   return "int64";
        case DataType::UInt64:  return "uint64";
        case DataType::Float:   return "float";
        case DataType::Double:  return "double";
        case DataType::Enum:    return "enum";
        case DataType::Vec2:    return "vec2";
        case DataType::Vec3:    return "vec3";
        case DataType::Vec4:    return "vec4";
        case DataType::Mat4:    return "mat4";
        case DataType::String:  return "string";
        case DataType::CharPtr: return "char*";
        case DataType::Key:     return "key";
        case DataType::Asset:   return "asset";
        case DataType::Array:   return "array";
        case DataType::Struct:  return "struct";
        case DataType::HexDump: return "hex";
        case DataType::Pad:     return "pad";
        default:                return "unknown";
    }
}

} // namespace GOW
