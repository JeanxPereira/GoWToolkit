#pragma once
#include "core/parsers/shared/MeshData.h"
#include "core/types/GameVersion.h"
#include "core/vfs/IFile.h"
#include <memory>

namespace GOW {

class GOW2MeshParser {
public:
    // Parse an MDL mesh block into a CPU-side MeshData structure
    static std::unique_ptr<MeshData> Parse(IFile& file, uint32_t offset, uint32_t size);

private:
    // allData = entire mesh blob, objOffset = object's position within mesh
    static bool ParseObject(const std::vector<uint8_t>& allData, uint32_t objOffset, uint32_t objSize, MeshData& outData);
    static bool ParseDmaChain(const std::vector<uint8_t>& allData, uint32_t objectOffset, uint32_t packetOffset, uint32_t dmaCount, MeshPart& outPart);
};

} // namespace GOW
