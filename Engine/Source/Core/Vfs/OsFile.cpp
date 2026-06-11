#include "OsFile.h"
#include <filesystem>

namespace Onyx::Vfs {

OsFile::OsFile(const std::string& path) {
    if (std::filesystem::exists(path)) {
        m_size = std::filesystem::file_size(path);
        // Open binary
        m_stream.open(path, std::ios::binary);
    }
}

OsFile::~OsFile() {
    if (m_stream.is_open()) m_stream.close();
}

bool OsFile::IsValid() const {
    return m_stream.is_open();
}

size_t OsFile::Read(void* dest, size_t bytes) {
    if (!m_stream.is_open() || m_stream.eof()) return 0;
    m_stream.read(reinterpret_cast<char*>(dest), bytes);
    return m_stream.gcount();
}

bool OsFile::Seek(int64_t offset, int origin) {
    if (!m_stream.is_open()) return false;
    std::ios_base::seekdir dir;
    if (origin == SEEK_SET) dir = std::ios::beg;
    else if (origin == SEEK_CUR) dir = std::ios::cur;
    else if (origin == SEEK_END) dir = std::ios::end;
    else return false;

    m_stream.clear(); // clear eof flags
    m_stream.seekg(offset, dir);
    return !m_stream.fail();
}

int64_t OsFile::Tell() {
    if (!m_stream.is_open()) return 0;
    return m_stream.tellg();
}

size_t OsFile::Size() const {
    return m_size;
}

bool OsFile::IsEOF() const {
    return m_stream.eof();
}

} // namespace Onyx::Vfs
