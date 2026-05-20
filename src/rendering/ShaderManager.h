#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

using GLuint = unsigned int;

namespace GOW {

/// Viewport shading mode
enum class ShadingMode {
    Solid,            // Blinn-Phong with textures + lighting
    Matcap,           // View-space normal matcap
    Textured,         // Material preview (game textures, minimal lighting)
    Wireframe,        // Matcap filled faces + Wireframe overlay
    TexturedWire      // Textured filled faces + Wireframe overlay
};

class Shader {
public:
    GLuint id = 0;

    void Use() const;
    void SetMat4(const char* name, const glm::mat4& mat) const;
    void SetMat3(const char* name, const glm::mat3& mat) const;
    void SetVec2(const char* name, const glm::vec2& v) const;
    void SetVec3(const char* name, const glm::vec3& v) const;
    void SetVec4(const char* name, const glm::vec4& v) const;
    void SetFloat(const char* name, float val) const;
    void SetInt(const char* name, int val) const;
};

class ShaderManager {
public:
    static ShaderManager& Get();

    void Initialize();
    Shader* GetShader(const std::string& name);

    /// Returns the procedurally generated matcap texture ID
    GLuint GetMatcapTexture() const { return m_matcapTexture; }

    void GenerateMatcapTexture();

private:
    ShaderManager() = default;
    GLuint CompileProgram(const char* vertSrc, const char* fragSrc);
    void GenerateBackgroundVAO();

    std::unordered_map<std::string, Shader> m_shaders;
    GLuint m_matcapTexture = 0;
    GLuint m_backgroundVAO = 0;  // Empty VAO for fullscreen triangle
    bool m_initialized = false;

    friend class SceneRenderer;
};

} // namespace GOW
