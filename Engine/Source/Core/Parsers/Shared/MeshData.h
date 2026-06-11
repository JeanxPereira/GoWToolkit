#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "Core/Domain/BoundingBox.h"
#include "Core/Domain/MeshVertex.h"

namespace Onyx::Parsers {

// A portion of a mesh using a single material
struct MeshPart {
    std::string name;                     // Descriptive name for the UI informer (e.g., Part_A)
    std::vector<Onyx::Domain::GpuVertex> vertices;
    std::vector<uint32_t>  indices;
    uint32_t materialId = 0;
    uint32_t textureLayer = 0;            // which DMA instance/layer this came from
    std::vector<uint16_t> jointMap;       // local joint index â†’ global skeleton joint
    bool useBindToJoint = true;           // true = use inverse bind pose (skinned)
    bool isSky = false;                   // true = this part should be rendered without writing depth
    bool isRigid = false;                 // GOWR: submesh has no BoneIdx/BoneWgt semantics; rigid-bound to jointMap[0]
    uint64_t meshHash = 0;                // GOWR LOD-blob identifier (0 = internal/embedded)
};

// The full CPU-side mesh representation
struct MeshData {
    std::vector<MeshPart> parts;
    Onyx::Domain::BoundingBox bounds;
};

} // namespace Onyx::Parsers
