#include <glad/glad.h>
#include "ShaderManager.h"
#include <glm/gtc/type_ptr.hpp>
#include "core/Logger.h"

namespace GOW {

// ── Shader uniform helpers ───────────────────────────────────────────────

void Shader::Use() const { glUseProgram(id); }

void Shader::SetMat4(const char* name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(mat));
}
void Shader::SetVec3(const char* name, const glm::vec3& v) const {
    glUniform3fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v));
}
void Shader::SetFloat(const char* name, float val) const {
    glUniform1f(glGetUniformLocation(id, name), val);
}
void Shader::SetInt(const char* name, int val) const {
    glUniform1i(glGetUniformLocation(id, name), val);
}

// ── Shader source code ──────────────────────────────────────────────────

static const char* DEFAULT_VERT = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;
out vec3 vFragPos;
out vec2 vUV;
out vec4 vColor;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal  = mat3(transpose(inverse(uModel))) * aNormal;
    vUV      = aUV;
    vColor   = aColor;
    gl_Position = uProjection * uView * worldPos;
}
)";

static const char* DEFAULT_FRAG = R"(
#version 330 core
in vec3 vNormal;
in vec3 vFragPos;
in vec2 vUV;
in vec4 vColor;

uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform int  uUseTexture;
uniform sampler2D uTexture0;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    if (!gl_FrontFacing) normal = -normal;

    // Ambient
    vec3 ambient = 0.15 * vec3(1.0);

    // Diffuse
    vec3 lightDir = normalize(uLightDir);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    // Specular (Blinn-Phong)
    vec3 viewDir  = normalize(uViewPos - vFragPos);
    vec3 halfDir  = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = 0.3 * spec * vec3(1.0);

    vec3 lighting = ambient + diffuse + specular;

    vec4 baseColor = vColor;
    if (uUseTexture == 1) {
        baseColor = texture(uTexture0, vUV) * vColor;
    }

    FragColor = vec4(lighting * baseColor.rgb, baseColor.a);
}
)";

static const char* GRID_VERT = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec4 vColor;
out float vDist;

void main() {
    vec4 worldPos = vec4(aPos, 1.0);
    vColor = aColor;
    vDist  = length(aPos.xz);
    gl_Position = uProjection * uView * worldPos;
}
)";

static const char* GRID_FRAG = R"(
#version 330 core
in vec4 vColor;
in float vDist;
out vec4 FragColor;

void main() {
    float fade = 1.0 - smoothstep(20.0, 50.0, vDist);
    FragColor = vec4(vColor.rgb, vColor.a * fade);
}
)";

// ── ShaderManager ────────────────────────────────────────────────────────

ShaderManager& ShaderManager::Get() {
    static ShaderManager instance;
    return instance;
}

GLuint ShaderManager::CompileProgram(const char* vertSrc, const char* fragSrc) {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            LOG_ERR("[Shader] Compile error: %s", log);
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        LOG_ERR("[Shader] Link error: %s", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void ShaderManager::Initialize() {
    if (m_initialized) return;
    m_initialized = true;

    Shader defaultShader;
    defaultShader.id = CompileProgram(DEFAULT_VERT, DEFAULT_FRAG);
    m_shaders["default"] = defaultShader;

    Shader gridShader;
    gridShader.id = CompileProgram(GRID_VERT, GRID_FRAG);
    m_shaders["grid"] = gridShader;

    LOG_INFO("[ShaderManager] Initialized %zu shaders.", m_shaders.size());
}

Shader* ShaderManager::GetShader(const std::string& name) {
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? &it->second : nullptr;
}

} // namespace GOW
