#pragma once
#include "DataTypes.h"
#include <unordered_map>
#include <string>

namespace GOW {

// ── AssetFormat ────────────────────────────────────────────────────────────
// Base class for all binary format definitions.
// Subclass and override Build() to declare the struct layout.
//
// Usage:
//   class GOW2Material : public AssetFormat {
//   protected:
//       void Build() override {
//           Struct("Material", 0x78,
//               Field("magic", DataType::UInt32, 0x0, DisplayHint::Hex),
//               Bool("alphaBlend", 0x4),
//               Vec4("diffuseColor", 0x10, DisplayHint::Color),
//               Str("texName", 0x30, 24)
//           );
//       }
//   };
class AssetFormat {
public:
    AssetFormat() = default;
    virtual ~AssetFormat() = default;

    // Must be called once before use (done automatically by static instances)
    void Initialize() {
        if (!m_built) { Build(); m_built = true; }
    }

    // Access parsed definitions
    const StructDefinition* GetStruct(const char* name) const {
        auto it = m_structs.find(name);
        return (it != m_structs.end()) ? &it->second : nullptr;
    }

    // Get the root struct (first registered)
    const StructDefinition* Root() const { return m_root; }

    // All registered structs
    const std::unordered_map<std::string, StructDefinition>& Structs() const { return m_structs; }

protected:
    virtual void Build() = 0;

    // ═══════════════════════════════════════════════════════════════════════
    // STRUCT REGISTRATION
    // ═══════════════════════════════════════════════════════════════════════

    template<typename... Fields>
    void Struct(const char* name, uint32_t size, Fields&&... fields) {
        StructDefinition def;
        def.name = name;
        def.size = size;
        (def.fields.push_back(std::forward<Fields>(fields)), ...);
        m_structs[name] = std::move(def);
        if (!m_root) m_root = &m_structs[name];
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FIELD HELPERS — Primitives
    // ═══════════════════════════════════════════════════════════════════════

    static FieldDef Field(const char* name, DataType type, uint32_t offset,
                          DisplayHint display = DisplayHint::Default) {
        FieldDef f{};
        f.name = name; f.type = type; f.offset = offset;
        f.size = DataTypeSize(type); f.display = display;
        return f;
    }

    static FieldDef Bool(const char* name, uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::Bool;
        f.offset = offset; f.size = 1;
        return f;
    }

    static FieldDef Int(const char* name, uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::Int32;
        f.offset = offset; f.size = 4;
        return f;
    }

    static FieldDef UInt(const char* name, uint32_t offset,
                         DisplayHint d = DisplayHint::Default) {
        FieldDef f{}; f.name = name; f.type = DataType::UInt32;
        f.offset = offset; f.size = 4; f.display = d;
        return f;
    }

    static FieldDef UInt16(const char* name, uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::UInt16;
        f.offset = offset; f.size = 2;
        return f;
    }

    static FieldDef UInt8(const char* name, uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::UInt8;
        f.offset = offset; f.size = 1;
        return f;
    }

    static FieldDef Float(const char* name, uint32_t offset,
                          DisplayHint d = DisplayHint::Default) {
        FieldDef f{}; f.name = name; f.type = DataType::Float;
        f.offset = offset; f.size = 4; f.display = d;
        return f;
    }

    static FieldDef Int64(const char* name, uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::Int64;
        f.offset = offset; f.size = 8;
        return f;
    }

    static FieldDef UInt64(const char* name, uint32_t offset,
                           DisplayHint d = DisplayHint::Default) {
        FieldDef f{}; f.name = name; f.type = DataType::UInt64;
        f.offset = offset; f.size = 8; f.display = d;
        return f;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FIELD HELPERS — Vectors
    // ═══════════════════════════════════════════════════════════════════════

    static FieldDef Vec2(const char* name, uint32_t offset,
                         DisplayHint d = DisplayHint::Default) {
        FieldDef f{}; f.name = name; f.type = DataType::Vec2;
        f.offset = offset; f.size = 8; f.display = d;
        return f;
    }

    static FieldDef Vec3(const char* name, uint32_t offset,
                         DisplayHint d = DisplayHint::Default) {
        FieldDef f{}; f.name = name; f.type = DataType::Vec3;
        f.offset = offset; f.size = 12; f.display = d;
        return f;
    }

    static FieldDef Vec4(const char* name, uint32_t offset,
                         DisplayHint d = DisplayHint::Default) {
        FieldDef f{}; f.name = name; f.type = DataType::Vec4;
        f.offset = offset; f.size = 16; f.display = d;
        return f;
    }

    static FieldDef Mat4(const char* name, uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::Mat4;
        f.offset = offset; f.size = 64;
        return f;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FIELD HELPERS — Strings, Keys, Assets
    // ═══════════════════════════════════════════════════════════════════════

    static FieldDef Str(const char* name, uint32_t offset, uint32_t maxLen) {
        FieldDef f{}; f.name = name; f.type = DataType::String;
        f.offset = offset; f.size = maxLen;
        return f;
    }

    static FieldDef Key(const char* name, uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::Key;
        f.offset = offset; f.size = 4; f.display = DisplayHint::Hex;
        return f;
    }

    static FieldDef Asset(const char* name, uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::Asset;
        f.offset = offset; f.size = 4;
        return f;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FIELD HELPERS — Enums
    // ═══════════════════════════════════════════════════════════════════════

    /// Enum with named lookup (register enum separately or use central EnumRegistry)
    static FieldDef EnumField(const char* name, const char* enumName,
                              uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::Enum;
        f.offset = offset; f.size = 4; f.enumName = enumName;
        return f;
    }

    /// Enum with inline value map (no registry needed)
    static FieldDef EnumFieldInline(const char* name, uint32_t offset,
                                    const EnumMap* map) {
        FieldDef f{}; f.name = name; f.type = DataType::Enum;
        f.offset = offset; f.size = 4; f.enumMap = map;
        return f;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FIELD HELPERS — Containers & Raw Data
    // ═══════════════════════════════════════════════════════════════════════

    /// Simple array of primitives
    static FieldDef Array(const char* name, DataType elemType,
                          uint32_t offset, uint32_t countOffset = 4) {
        FieldDef f{}; f.name = name; f.type = DataType::Array;
        f.offset = offset; f.size = 0; f.countOffset = countOffset;
        return f;
    }

    /// Array of nested structs
    static FieldDef ArrayStruct(const char* name, const char* structName,
                                uint32_t offset, uint32_t countOffset = 4) {
        FieldDef f{}; f.name = name; f.type = DataType::Array;
        f.offset = offset; f.size = 0; f.structName = structName;
        f.countOffset = countOffset;
        return f;
    }

    /// Nested inline struct
    static FieldDef IStruct(const char* name, const char* structName,
                            uint32_t offset) {
        FieldDef f{}; f.name = name; f.type = DataType::Struct;
        f.offset = offset; f.size = 0; f.structName = structName;
        return f;
    }

    /// Raw hex dump
    static FieldDef Hex(const char* name, uint32_t offset, uint32_t bytes) {
        FieldDef f{}; f.name = name; f.type = DataType::HexDump;
        f.offset = offset; f.size = bytes;
        return f;
    }

    /// Padding (hidden in inspector)
    static FieldDef Pad(uint32_t offset, uint32_t bytes) {
        FieldDef f{}; f.name = "_pad"; f.type = DataType::Pad;
        f.offset = offset; f.size = bytes; f.display = DisplayHint::Hidden;
        return f;
    }

private:
    std::unordered_map<std::string, StructDefinition> m_structs;
    const StructDefinition* m_root = nullptr;
    bool m_built = false;
};

} // namespace GOW
