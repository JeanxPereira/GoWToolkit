#include "IsoFile.h"

namespace GOW {

IsoFile::IsoFile(const std::string& isoPath, uint64_t startOffset, size_t size) 
    : m_startOffset(startOffset), m_size(size), m_pos(0) 
{
    m_stream.open(isoPath, std::ios::binary);
    if (m_stream.is_open()) {
        m_stream.seekg(m_startOffset, std::ios::beg);
    }
}

IsoFile::~IsoFile() {
    if (m_stream.is_open()) m_stream.close();
}

bool IsoFile::IsValid() const {
    return m_stream.is_open() && m_stream.good();
}

size_t IsoFile::Read(void* dest, size_t bytes) {
    if (!m_stream.is_open() || static_cast<size_t>(m_pos) >= m_size) return 0;
    
    size_t toRead = bytes;
    if (m_pos + toRead > m_size) {
        toRead = m_size - m_pos;
    }
    
    m_stream.read(reinterpret_cast<char*>(dest), toRead);
    size_t readCount = m_stream.gcount();
    m_pos += readCount;
    return readCount;
}

bool IsoFile::Seek(int64_t offset, int origin) {
    int64_t newPos = m_pos;
    if (origin == SEEK_SET) newPos = offset;
    else if (origin == SEEK_CUR) newPos += offset;
    else if (origin == SEEK_END) newPos = m_size + offset; // offset is usually negative here
    else return false;
    
    // Allow seeking to EOF (newPos == m_size)
    if (newPos < 0 || static_cast<size_t>(newPos) > m_size) return false;
    
    m_pos = newPos;
    m_stream.clear(); // Clear EOF
    m_stream.seekg(m_startOffset + m_pos, std::ios::beg);
    return !m_stream.fail();
}

int64_t IsoFile::Tell() {
    return m_pos;
}

size_t IsoFile::Size() const {
    return m_size;
}

bool IsoFile::IsEOF() const {
    return static_cast<size_t>(m_pos) >= m_size;
}

} // namespace GOW
