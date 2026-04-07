#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace GOW {

class IFile {
public:
    virtual ~IFile() = default;

    virtual size_t Read(void* dest, size_t bytes) = 0;
    virtual bool Seek(int64_t offset, int origin) = 0; // origin: SEEK_SET, SEEK_CUR, SEEK_END
    virtual int64_t Tell() = 0;
    virtual size_t Size() const = 0;
    virtual bool IsEOF() const = 0;
    virtual bool IsValid() const = 0;
    
    // Ler o arquivo inteiro para a memória de uma vez pode ser útil para pequenos pedaços
    virtual std::vector<uint8_t> ReadAll() {
        size_t size = Size();
        std::vector<uint8_t> buffer(size);
        Seek(0, 0); // SEEK_SET
        Read(buffer.data(), size);
        return buffer;
    }
};

} // namespace GOW
