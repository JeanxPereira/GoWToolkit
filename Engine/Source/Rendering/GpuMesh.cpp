#include <glad/glad.h>
#include "GpuMesh.h"
#include <algorithm>
#include <limits>

namespace Onyx {

GpuMesh::~GpuMesh() { Cleanup(); }

GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo),
      m_indexCount(other.m_indexCount), m_vertexCount(other.m_vertexCount),
      m_bounds(other.m_bounds) {
    other.m_vao = other.m_vbo = other.m_ebo = 0;
}

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_vao = other.m_vao; m_vbo = other.m_vbo; m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount; m_vertexCount = other.m_vertexCount;
        m_bounds = other.m_bounds;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
    }
    return *this;
}

void GpuMesh::Cleanup() {
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao = m_vbo = m_ebo = 0;
}

void GpuMesh::Upload(const std::vector<Domain::GpuVertex>& vertices,
                     const std::vector<uint32_t>& indices) {
    Cleanup();

    m_vertexCount = (int)vertices.size();
    m_indexCount  = (int)indices.size();

    // Compute bounding box
    glm::vec3 bmin(std::numeric_limits<float>::max());
    glm::vec3 bmax(std::numeric_limits<float>::lowest());
    for (auto& v : vertices) {
        bmin = glm::min(bmin, v.position);
        bmax = glm::max(bmax, v.position);
    }
    m_bounds.min = bmin;
    m_bounds.max = bmax;

    // Create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Domain::GpuVertex),
                 vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(uint32_t),
                 indices.data(), GL_STATIC_DRAW);

    // Vertex attributes (match shader layout)
    // location 0: position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Domain::GpuVertex),
                          (void*)offsetof(Domain::GpuVertex, position));
    glEnableVertexAttribArray(0);

    // location 1: normal (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Domain::GpuVertex),
                          (void*)offsetof(Domain::GpuVertex, normal));
    glEnableVertexAttribArray(1);

    // location 2: uv (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Domain::GpuVertex),
                          (void*)offsetof(Domain::GpuVertex, uv));
    glEnableVertexAttribArray(2);

    // location 3: color (vec4)
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Domain::GpuVertex),
                          (void*)offsetof(Domain::GpuVertex, color));
    glEnableVertexAttribArray(3);

    // location 4: uv1 (vec2) — secondary UV for env mapping
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Domain::GpuVertex),
                          (void*)offsetof(Domain::GpuVertex, uv1));
    glEnableVertexAttribArray(4);

    // location 5: boneWeights (vec4)
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Domain::GpuVertex),
                          (void*)offsetof(Domain::GpuVertex, boneWeights));
    glEnableVertexAttribArray(5);

    // location 6: boneIndices (uvec4) — integer attribute
    glVertexAttribIPointer(6, 4, GL_UNSIGNED_INT, sizeof(Domain::GpuVertex),
                           (void*)offsetof(Domain::GpuVertex, boneIndices));
    glEnableVertexAttribArray(6);

    glBindVertexArray(0);
}

void GpuMesh::Draw() const {
    if (m_vao == 0 || m_indexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace Onyx
