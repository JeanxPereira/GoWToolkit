#pragma once
#include <glm/glm.hpp>

namespace Onyx::Domain {

/// CPU-side vertex used by mesh parsers and uploaded to the GPU as-is.
/// POD layout — `offsetof` is relied on by `rendering/GpuMesh.cpp` for
/// glVertexAttribPointer setup. Do not change field order without updating
/// the GL attribute table in lockstep.
struct GpuVertex {
    glm::vec3  position    {0.0f};
    glm::vec3  normal      {0.0f};
    glm::vec2  uv          {0.0f};
    glm::vec4  color       {1.0f};
    // ── GOWR additions (zero-initialised; GOW2 paths never write these) ─
    glm::vec2  uv1         {0.0f};
    glm::vec4  boneWeights {0.0f};
    glm::uvec4 boneIndices {0u};
};

} // namespace Onyx::Domain

// Backwards-compat alias
namespace Onyx { using GpuVertex = Domain::GpuVertex; }
