#pragma once
// ── MeshParser.h ───────────────────────────────────────────────────────────
#include "core/parsers/shared/MeshData.h"
#include "core/vfs/IFile.h"
#include "LodPackIndex.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace GOW {

class GOWRMeshParser {
public:
    // ── Public API ────────────────────────────────────────────────────────

    // Full parse from a pre-resolved GPU file (hash=0 submeshes, or already
    // externally resolved LOD blobs). Does NOT do lodpack lookup.
    static bool Parse(std::shared_ptr<IFile> meshFile,
                      std::shared_ptr<IFile> gpuFile,
                      MeshData& outData);

    // Full parse with lodpack lookup.
    // For each submesh: if meshHash != 0 → reads blob from lodIdx;
    //                   if meshHash == 0 → uses gpuFile directly.
    // This is the primary production entry point.
    static bool ParseWithLodPack(std::shared_ptr<IFile>    meshFile,
                                  std::shared_ptr<IFile>    gpuFile,
                                  const LodPackIndex&       lodIdx,
                                  MeshData&                 outData);

    // Header-only parse (no GPU read, for tree inspection)
    static bool ParseMeshDefn(std::shared_ptr<IFile> defFile,
                               std::shared_ptr<IFile> lodpackFile,
                               std::vector<std::shared_ptr<GpuMesh>>& outMeshes);

    // ── Public enums ──────────────────────────────────────────────────────

    enum class Semantic : uint8_t {
        Position = 0,
        Normal   = 1,
        Tangent  = 2,
        UV0      = 3,   // primary UV  (C# UV1)
        UV1      = 4,   // lightmap UV (C# UV2)
        UV2      = 5,
        UV3      = 6,
        BoneIdx  = 9,
        BoneWgt  = 10,
    };

    enum class AttrFormat : uint8_t {
        Float32   = 0,  // N × float32
        R10G10B10 = 3,  // packed uint32, 10 bits per channel
        Uint16    = 6,  // N × uint16, unorm (value/65535 for UV, /32768-1 for position)
        Int16     = 7,  // N × int16 snorm, /32767
        Uint8     = 8,  // N × uint8
    };

private:
    // ── Internal structures ───────────────────────────────────────────────

    struct ComponentDesc {
        Semantic   semantic;
        AttrFormat format;
        uint8_t    compCount;   // number of components (1-4)
        uint8_t    byteOffset;  // byte offset within the interleaved stride
        uint32_t   bufferIdx;   // index into the buffer-offsets table
    };

    struct SubmeshHeader {
        glm::vec3 extent;   // per-axis scale  (dequantisation)
        glm::vec3 origin;   // per-axis bias   (dequantisation)

        uint32_t vertCount;
        uint32_t faceCount;
        uint32_t indCount;

        uint32_t componentOffsetAbs;  // absolute position in MESH file
        uint32_t bufOffsetsAbs;       // absolute position in MESH file
        uint32_t gpuIndexOffset;      // absolute offset inside GPU/LOD file

        uint64_t meshHash;

        uint8_t bufferCount;
        uint8_t indicesStride;    // 2 = uint16, 4 = uint32
        uint8_t bytesPerVertex;   // interleaved stride (valid when bufferCount == 1)
        uint8_t componentCount;
    };

    // ── Private helpers ───────────────────────────────────────────────────

    static bool ReadSubmeshHeader(std::shared_ptr<IFile>& meshFile,
                                  uint32_t submeshBase,
                                  SubmeshHeader& out);

    static bool ReadComponents(std::shared_ptr<IFile>& meshFile,
                               const SubmeshHeader& hdr,
                               std::vector<ComponentDesc>& out);

    static bool ReadBufferOffsets(std::shared_ptr<IFile>& meshFile,
                                  const SubmeshHeader& hdr,
                                  std::vector<uint32_t>& out);

    static bool ReadVertices(std::shared_ptr<IFile>& gpuFile,
                             const SubmeshHeader& hdr,
                             const std::vector<ComponentDesc>& comps,
                             const std::vector<uint32_t>& bufOffsets,
                             MeshPart& outPart);

    static bool ReadIndices(std::shared_ptr<IFile>& gpuFile,
                            const SubmeshHeader& hdr,
                            MeshPart& outPart);
};

} // namespace GOW
