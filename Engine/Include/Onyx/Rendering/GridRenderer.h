#pragma once
#include <glm/glm.hpp>

using GLuint = unsigned int;

namespace Onyx::Rendering {

class Shader;

class GridRenderer {
public:
    GridRenderer() = default;
    ~GridRenderer();

    void Initialize();
    void Draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, const glm::vec4& gridColor, float gridScale = 1.0f);

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int    m_vertexCount = 0;
    bool   m_initialized = false;
};

} // namespace Onyx::Rendering
