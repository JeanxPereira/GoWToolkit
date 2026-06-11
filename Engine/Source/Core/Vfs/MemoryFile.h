#pragma once
#include "IFile.h"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>

namespace Onyx::Vfs {

class MemoryFile : public IFile {
public:
    MemoryFile(std::vector<uint8_t> data) : m_data(std::move(data)), m_pos(0) {}

    size_t Read(void* dest, size_t bytes) override {
        if (m_pos >= m_data.size()) return 0;
        size_t available = m_data.size() - m_pos;
        size_t toRead = std::min(bytes, available);
        std::memcpy(dest, m_data.data() + m_pos, toRead);
        m_pos += toRead;
        return toRead;
    }

    bool Seek(int64_t offset, int origin) override {
        int64_t newPos = 0;
        if (origin == 0) { // SEEK_SET
            newPos = offset;
        } else if (origin == 1) { // SEEK_CUR
            newPos = m_pos + offset;
        } else if (origin == 2) { // SEEK_END
            newPos = m_data.size() + offset;
        } else {
            return false;
        }

        if (newPos < 0 || newPos > (int64_t)m_data.size()) return false;
        m_pos = (size_t)newPos;
        return true;
    }

    int64_t Tell() override { return m_pos; }
    size_t Size() const override { return m_data.size(); }
    bool IsEOF() const override { return m_pos >= m_data.size(); }
    bool IsValid() const override { return true; }

private:
    std::vector<uint8_t> m_data; // take ownership by value
    size_t m_pos;
};

} // namespace Onyx::Vfs
