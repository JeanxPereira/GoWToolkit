#include "NodeInstance.h"
#include "../vfs/IFile.h"

namespace Onyx {

bool NodeInstance::ReadFromFile(IFile* file) {
    if (!file || !m_def) return false;
    
    size_t sizeToRead = m_def->GetFixedSize();
    if (sizeToRead == 0) return true; // Noting to read or purely variable (unhandled by this method)

    if (m_data.size() < sizeToRead) {
        m_data.resize(sizeToRead);
    }

    size_t readCount = file->Read(m_data.data(), sizeToRead);
    return readCount == sizeToRead;
}

} // namespace Onyx
