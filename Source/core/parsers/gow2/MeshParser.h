#pragma once
#include <Onyx/Parsers/MeshData.h>
#include <Onyx/Vfs/IFile.h>
#include <memory>

namespace Onyx {

class GOW2MeshParser {
public:
    // Parse an MDL mesh block into a CPU-side Parsers::MeshData structure
    static std::unique_ptr<Parsers::MeshData> Parse(Vfs::IFile& file, uint32_t offset, uint32_t size);

private:
    // allData = entire mesh blob, objOffset = object's position within mesh
    static bool ParseObject(const std::vector<uint8_t>& allData, uint32_t objOffset, uint32_t objSize, Parsers::MeshData& outData);
    static bool ParseDmaChain(const std::vector<uint8_t>& allData, uint32_t objectOffset, uint32_t packetOffset, uint32_t dmaCount, Parsers::MeshPart& outPart);
};

} // namespace Onyx
