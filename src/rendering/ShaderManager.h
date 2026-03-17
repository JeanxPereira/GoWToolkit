#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

using GLuint = unsigned int;

namespace GOW {

class Shader {
public:
    GLuint id = 0;

    void Use() const;
    void SetMat4(const char* name, const glm::mat4& mat) const;
    void SetVec3(const char* name, const glm::vec3& v) const;
    void SetFloat(const char* name, float val) const;
    void SetInt(const char* name, int val) const;
};

class ShaderManager {
public:
    static ShaderManager& Get();

    void Initialize();
    Shader* GetShader(const std::string& name);

private:
    ShaderManager() = default;
    GLuint CompileProgram(const char* vertSrc, const char* fragSrc);

    std::unordered_map<std::string, Shader> m_shaders;
    bool m_initialized = false;
};

} // namespace GOW
