#pragma once
#include <Onyx/Schema/DataSchema.h>
#include <vector>
#include <string>
#include <memory>

namespace Onyx::Schema {

class StructDef {
public:
    StructDef(std::string name) : m_name(std::move(name)) {}

    // Chainable builder methods
    StructDef& AddField(std::string name, FieldType type, size_t count = 1) {
        m_fields.emplace_back(std::move(name), type, count);
        return *this;
    }

    StructDef& AddUInt16(std::string name) { return AddField(std::move(name), FieldType::UInt16); }
    StructDef& AddInt32(std::string name) { return AddField(std::move(name), FieldType::Int32); }
    StructDef& AddUInt32(std::string name) { return AddField(std::move(name), FieldType::UInt32); }
    StructDef& AddFloat(std::string name)  { return AddField(std::move(name), FieldType::Float); }
    StructDef& AddVector3(std::string name) { return AddField(std::move(name), FieldType::Vector3); }
    StructDef& AddVector4(std::string name) { return AddField(std::move(name), FieldType::Vector4); }
    StructDef& AddMatrix4x4(std::string name) { return AddField(std::move(name), FieldType::Matrix4x4); }
    StructDef& AddHexDump(std::string name, size_t bytes) { return AddField(std::move(name), FieldType::HexDump, bytes); }

    const std::string& GetName() const { return m_name; }
    const std::vector<SchemaField>& GetFields() const { return m_fields; }

    // Calcula o tamanho em bytes dessa struct inteira (baseado em campos de tamanho fixo)
    size_t GetFixedSize() const;

private:
    std::string m_name;
    std::vector<SchemaField> m_fields;
};

} // namespace Onyx::Schema
