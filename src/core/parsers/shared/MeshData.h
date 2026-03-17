#pragma once
#include <vector>
#include <cstdint>
#include "rendering/GpuMesh.h" // For BoundingBox and GpuVertex

namespace GOW {

// A portion of a mesh using a single material
struct MeshPart {
    std::vector<GpuVertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t materialId = 0;
};

// The full CPU-side mesh presentation
struct MeshData {
    std::vector<MeshPart> parts;
    BoundingBox bounds;
};

} // namespace GOW
