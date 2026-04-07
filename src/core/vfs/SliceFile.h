#pragma once
#include "IFile.h"
#include <memory>
#include <stdexcept>
#include <algorithm>

namespace GOW {

// Encapsula outro IFile garantindo que não passemos do offset inicial + tamanho.
// Essencial para ler WADs e entradas contíguas numa ISO.
class SliceFile : public IFile {
public:
    SliceFile(std::shared_ptr<IFile> parent, size_t startOffset, size_t size)
        : m_parent(parent), m_start(startOffset), m_size(size), m_pos(0) {
        if (m_parent) {
            m_parent->Seek(m_start, SEEK_SET);
        }
    }

    size_t Read(void* buffer, size_t size) override {
        if (!m_parent) return 0;
        if (m_pos >= m_size) return 0;

        size_t available = m_size - m_pos;
        size_t toRead = std::min(size, available);
        
        m_parent->Seek(m_start + m_pos, SEEK_SET);
        size_t readCount = m_parent->Read(buffer, toRead);
        m_pos += readCount;
        return readCount;
    }

    bool Seek(int64_t offset, int origin) override {
        if (!m_parent) return false;
        int64_t newPos = (int64_t)m_pos;
        
        if (origin == SEEK_SET) newPos = offset;
        else if (origin == SEEK_CUR) newPos += offset;
        else if (origin == SEEK_END) newPos = (int64_t)m_size + offset;

        if (newPos < 0 || newPos > (int64_t)m_size) return false;
        m_pos = (size_t)newPos;
        return m_parent->Seek(m_start + m_pos, SEEK_SET);
    }

    int64_t Tell() override {
        return m_pos;
    }

    size_t Size() const override {
        return m_size;
    }

    bool IsValid() const override {
        return m_parent && m_parent->IsValid();
    }

    bool IsEOF() const override {
        return m_pos >= m_size || (m_parent && m_parent->IsEOF());
    }

private:
    std::shared_ptr<IFile> m_parent;
    size_t m_start;
    size_t m_size;
    size_t m_pos;
};

} // namespace GOW
