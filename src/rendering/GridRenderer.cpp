#include <glad/glad.h>
#include "GridRenderer.h"
#include "ShaderManager.h"
#include "core/Logger.h"
#include <vector>

namespace GOW {

GridRenderer::~GridRenderer() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void GridRenderer::Initialize() {
    if (m_initialized) return;
    m_initialized = true;

    struct GridVert { glm::vec3 pos; glm::vec4 color; };
    std::vector<GridVert> verts;

    const float SPACING = 1000.0f;     // 1000 units between lines
    const int   HALF_LINES = 100;      // 100 lines each side = extends to ±100000
    const float EXTENT = HALF_LINES * SPACING;
    const glm::vec4 gridColor(0.35f, 0.35f, 0.35f, 0.5f);
    const glm::vec4 xAxisColor(0.8f, 0.2f, 0.2f, 0.8f);
    const glm::vec4 zAxisColor(0.2f, 0.4f, 0.8f, 0.8f);

    for (int i = -HALF_LINES; i <= HALF_LINES; i++) {
        float pos = i * SPACING;
        // Lines parallel to Z (varying X)
        glm::vec4 col = (i == 0) ? xAxisColor : gridColor;
        verts.push_back({glm::vec3(pos, 0, -EXTENT), col});
        verts.push_back({glm::vec3(pos, 0,  EXTENT), col});

        // Lines parallel to X (varying Z)
        col = (i == 0) ? zAxisColor : gridColor;
        verts.push_back({glm::vec3(-EXTENT, 0, pos), col});
        verts.push_back({glm::vec3( EXTENT, 0, pos), col});
    }

    m_vertexCount = (int)verts.size();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GridVert), verts.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVert), (void*)0);
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GridVert), (void*)offsetof(GridVert, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GridRenderer::Draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec4& gridColor) {
    if (!m_initialized) {
        Initialize();
        LOG_INFO("[GridRenderer] Initialized: VAO=%u VBO=%u vertexCount=%d", m_vao, m_vbo, m_vertexCount);
    }

    auto* shader = ShaderManager::Get().GetShader("grid");
    if (!shader) {
        LOG_INFO("[GridRenderer] Grid shader not found!");
        return;
    }

    shader->Use();
    shader->SetMat4("uView", view);
    shader->SetMat4("uProjection", projection);
    shader->SetVec4("uGridColor", gridColor);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, m_vertexCount);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

} // namespace GOW
