#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include "core/domain/BoundingBox.h"

// Forward-declare GL types to avoid pulling glad.h into headers
using GLuint = unsigned int;

namespace GOW {

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

class GpuMesh {
public:
    GpuMesh() = default;
    ~GpuMesh();

    // Non-copyable, movable
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;

    void Upload(const std::vector<GpuVertex>& vertices,
                const std::vector<uint32_t>& indices);
    void Draw() const;

    void SetTexture(GLuint texId) { m_textureId = texId; }
    bool HasTexture() const { return m_textureId != 0; }
    GLuint GetTexture() const { return m_textureId; }

    int GetIndexCount() const { return m_indexCount; }
    int GetVertexCount() const { return m_vertexCount; }
    BoundingBox GetBounds() const { return m_bounds; }

private:
    void Cleanup();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    int    m_indexCount  = 0;
    int    m_vertexCount = 0;
    BoundingBox m_bounds;
    GLuint m_textureId = 0;
};

} // namespace GOW