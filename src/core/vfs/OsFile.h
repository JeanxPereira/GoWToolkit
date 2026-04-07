#pragma once
#include "IFile.h"
#include <fstream>
#include <string>

namespace GOW {

class OsFile : public IFile {
public:
    OsFile(const std::string& path);
    ~OsFile() override;

    bool IsValid() const override;

    size_t Read(void* dest, size_t bytes) override;
    bool Seek(int64_t offset, int origin) override;
    int64_t Tell() override;
    size_t Size() const override;
    bool IsEOF() const override;

private:
    std::ifstream m_stream;
    size_t m_size = 0;
};

} // namespace GOW
