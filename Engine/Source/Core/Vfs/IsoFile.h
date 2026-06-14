#pragma once
#include <Onyx/Vfs/IFile.h>
#include <fstream>
#include <string>

namespace Onyx::Vfs {

class IsoFile : public IFile {
public:
    IsoFile(const std::string& isoPath, uint64_t startOffset, size_t size);
    ~IsoFile() override;
    
    bool IsValid() const;

    size_t Read(void* dest, size_t bytes) override;
    bool Seek(int64_t offset, int origin) override;
    int64_t Tell() override;
    size_t Size() const override;
    bool IsEOF() const override;

private:
    std::ifstream m_stream;
    uint64_t m_startOffset;
    size_t m_size;
    int64_t m_pos; // Logical position within the subfile
};

} // namespace Onyx::Vfs
