#include "MeshParser.h"
#include "core/Logger.h"
#include <cstring>

namespace GOW {

bool GOWRMeshParser::ParseMeshDefn(std::shared_ptr<IFile> defFile, std::shared_ptr<IFile> lodpackFile, std::vector<std::shared_ptr<GpuMesh>>& outMeshes) {
    if (!defFile || !defFile->IsValid()) return false;

    defFile->Seek(0, SEEK_SET);

    uint32_t magic;
    if (defFile->Read(&magic, 4) != 4 || magic != 655372) {
        LOG_ERR("[GOWRMeshParser] Invalid magic (expected 655372, got %u)", magic);
        return false;
    }

    defFile->Seek(0x0C, SEEK_SET);
    uint32_t defOffsetsBegin;
    defFile->Read(&defOffsetsBegin, 4);

    uint32_t defCount;
    defFile->Read(&defCount, 4);

    LOG_INFO("[GOWRMeshParser] Found %u mesh definitions at offset 0x%X", defCount, defOffsetsBegin);

    // Read offsets vector
    std::vector<uint32_t> defOffsets(defCount);
    defFile->Seek(defOffsetsBegin, SEEK_SET);
    for (uint32_t i = 0; i < defCount; ++i) {
        defFile->Read(&defOffsets[i], 4);
        defOffsets[i] += defOffsetsBegin;
    }

    // Since we don't have automatic external loading of .lodpack yet, we will just parse the headers
    // and print them if no lodpack is provided.

    for (uint32_t i = 0; i < defCount; ++i) {
        defFile->Seek(defOffsets[i] + 0x10, SEEK_SET);

        glm::vec3 extent;
        defFile->Read(&extent, 12);

        defFile->Seek(defOffsets[i] + 0x18 + 0x04, SEEK_SET);
        glm::vec3 origin;
        defFile->Read(&origin, 12);

        defFile->Seek(defOffsets[i] + 0x44, SEEK_SET);
        uint32_t vertCount;
        uint32_t faceCount;
        defFile->Read(&vertCount, 4);
        defFile->Read(&faceCount, 4);

        defFile->Seek(defOffsets[i] + 0x54, SEEK_SET);
        uint32_t indCount;
        uint32_t componentOffset;
        uint32_t bufferOffsetsOffset;
        defFile->Read(&indCount, 4);
        defFile->Read(&componentOffset, 4);
        defFile->Read(&bufferOffsetsOffset, 4);
        
        uint64_t meshHash;
        defFile->Read(&meshHash, 8);

        uint8_t bufferCount;
        uint8_t indicesStride;
        uint8_t bytesPerVertex;
        uint8_t componentCount;
        defFile->Read(&bufferCount, 1);
        defFile->Read(&indicesStride, 1);
        defFile->Read(&bytesPerVertex, 1);
        defFile->Read(&componentCount, 1);

        LOG_INFO("[GOWRMeshParser] Mesh #%u: Extents(%.2f,%.2f,%.2f), Origin(%.2f,%.2f,%.2f)", 
                 i, extent.x, extent.y, extent.z, origin.x, origin.y, origin.z);
        LOG_INFO("[GOWRMeshParser]   %u Vertices, %u Indices (Stride:%u), %u Faces, Hash 0x%016llX",
                 vertCount, indCount, indicesStride, faceCount, meshHash);
        LOG_INFO("[GOWRMeshParser]   %u Components, %u Vertex Buffers, BPV=%u",
                 componentCount, bufferCount, bytesPerVertex);
                 
        // In the future this will read component lists and construct the GpuMesh
    }

    return true;
}

} // namespace GOW
