#pragma once
#include "StructDef.h"
#include <vector>
#include <memory>
#include <cstddef>
#include <stdexcept>
#include <cstring>

namespace Onyx {

class NodeInstance {
public:
    NodeInstance(std::shared_ptr<StructDef> def) : m_def(std::move(def)) {
        if (m_def) {
            m_data.resize(m_def->GetFixedSize());
        }
    }

    std::shared_ptr<StructDef> GetDef() const { return m_def; }
    const std::vector<uint8_t>& GetData() const { return m_data; }
    std::vector<uint8_t>& GetData() { return m_data; }

    // Read/Write access by offset
    template<typename T>
    T GetValue(size_t offset) const {
        if (offset + sizeof(T) > m_data.size()) throw std::out_of_range("NodeInstance::GetValue OOB");
        T val;
        std::memcpy(&val, m_data.data() + offset, sizeof(T));
        return val;
    }

    template<typename T>
    void SetValue(size_t offset, const T& val) {
        if (offset + sizeof(T) > m_data.size()) throw std::out_of_range("NodeInstance::SetValue OOB");
        std::memcpy(m_data.data() + offset, &val, sizeof(T));
    }

    // Leitura a partir de um File Stream (raw reading for fixed size structs)
    bool ReadFromFile(class IFile* file);

private:
    std::shared_ptr<StructDef> m_def;
    std::vector<uint8_t> m_data;
};

} // namespace Onyx
