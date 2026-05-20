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

    // We no longer need VBOs because the shader generates a fullscreen triangle 
    // procedurally using gl_VertexID. We only need an empty VAO to satisfy OpenGL core profile.
    glGenVertexArrays(1, &m_vao);
    m_vertexCount = 3;
}

void GridRenderer::Draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, const glm::vec4& gridColor, float gridScale) {
    if (!m_initialized) {
        Initialize();
        LOG_INFO("[GridRenderer] Initialized (Fullscreen Triangle): VAO=%u", m_vao);
    }

    auto* shader = ShaderManager::Get().GetShader("grid");
    if (!shader) {
        LOG_INFO("[GridRenderer] Grid shader not found!");
        return;
    }

    shader->Use();
    
    glm::mat4 viewProj = projection * view;
    shader->SetMat4("uViewProj", viewProj);
    shader->SetMat4("uInvViewProj", glm::inverse(viewProj));
    
    shader->SetVec4("uGridColor", gridColor);
    shader->SetVec3("uCameraPos", cameraPos);
    shader->SetFloat("uGridScale", gridScale);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable depth testing but don't write to depth buffer (so transparent grid blends over background)
    // Actually, the shader writes to gl_FragDepth so it properly occludes with the 3D models.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

} // namespace GOW
