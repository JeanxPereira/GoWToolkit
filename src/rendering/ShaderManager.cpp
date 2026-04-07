#include <glad/glad.h>
#include "ShaderManager.h"
#include "core/AppConfig.h"
#include <glm/gtc/type_ptr.hpp>
#include "core/Logger.h"
#include <cmath>
#include <vector>

namespace GOW {

// ── Shader uniform helpers ───────────────────────────────────────────────

void Shader::Use() const { glUseProgram(id); }

void Shader::SetMat4(const char* name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(mat));
}
void Shader::SetMat3(const char* name, const glm::mat3& mat) const {
    glUniformMatrix3fv(glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(mat));
}
void Shader::SetVec3(const char* name, const glm::vec3& v) const {
    glUniform3fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v));
}
void Shader::SetVec4(const char* name, const glm::vec4& v) const {
    glUniform4fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v));
}
void Shader::SetFloat(const char* name, float val) const {
    glUniform1f(glGetUniformLocation(id, name), val);
}
void Shader::SetInt(const char* name, int val) const {
    glUniform1i(glGetUniformLocation(id, name), val);
}

// ═══════════════════════════════════════════════════════════════════════════
// SHADER SOURCES
// ═══════════════════════════════════════════════════════════════════════════

// ── Unified Scene Shader ────────────────────────────────────────────────
// Handles all shading modes: Solid (Blinn-Phong), Matcap, Textured (material preview).
// Supports skeletal animation, multi-layer materials, vertex colors.

static const char* SCENE_VERT = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aColor;
layout(location=4) in vec2 aUV1;
layout(location=5) in vec4 aBoneWeights;
layout(location=6) in uvec4 aBoneIndices;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModelTransform;
uniform mat4 uJoints[150];
uniform int  uUseJoints;
uniform vec2 uLayerOffset;

out vec3 vWorldPos;
out vec3 vWorldNormal;
out vec3 vViewNormal;
out vec2 vUV;
out vec2 vUV1;
out vec4 vColor;
out float vDet;

void main() {
    vec4 localPos;
    vec3 localNormal;

    if (uUseJoints == 1) {
        // GOW2 skinning — porta exata do export_gltf.go:
        //   Weight (flags & 0x7fff / 4096)  → peso do jointIndexes[0] (boneIndices.x)
        //   1 - Weight                       → peso do jointIndexes[1] (boneIndices.y)
        //
        // Quando Weight == 0: 100% boneIndices.y (joint secundário é o único ativo)
        // Quando Weight == 1: 100% boneIndices.x (joint primário é o único ativo)
        // NUNCA usar uJoints[0] como fallback — isso colapsa tudo na raiz.
        float w0 = aBoneWeights.x;  // Weight → peso de boneIndices.x
        float w1 = aBoneWeights.y;  // 1-Weight → peso de boneIndices.y
        if (w0 > 0.001 && w1 > 0.001) {
            // Blended: dois joints ativos
            mat4 skin = uJoints[aBoneIndices.x] * w0
                      + uJoints[aBoneIndices.y] * w1;
            localPos    = skin * vec4(aPos, 1.0);
            localNormal = mat3(skin) * aNormal;
        } else if (w0 > 0.001) {
            // Rigid: 100% boneIndices.x
            localPos    = uJoints[aBoneIndices.x] * vec4(aPos, 1.0);
            localNormal = mat3(uJoints[aBoneIndices.x]) * aNormal;
        } else {
            // Rigid: 100% boneIndices.y (Weight == 0)
            localPos    = uJoints[aBoneIndices.y] * vec4(aPos, 1.0);
            localNormal = mat3(uJoints[aBoneIndices.y]) * aNormal;
        }
    } else {
        localPos    = vec4(aPos, 1.0);
        localNormal = aNormal;
    }

    // Always apply model transform (handles Z-flip for GOW2 models)
    vec4 worldPos   = uModelTransform * localPos;
    vec3 worldNormal = mat3(uModelTransform) * localNormal;
    vDet = determinant(mat3(uModelTransform));

    vWorldPos    = worldPos.xyz;
    vWorldNormal = normalize(worldNormal);
    vViewNormal  = mat3(uView) * vWorldNormal;

    vUV  = aUV + uLayerOffset;
    vUV1 = aUV1;

    // PS2 vertex color: 128 = full brightness, allow overbright
    vColor   = aColor * 2.0;
    vColor.a = clamp(aColor.a, 0.0, 1.0);

    gl_Position = uProjection * uView * worldPos;
}
)";

static const char* SCENE_FRAG = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vWorldNormal;
in vec3 vViewNormal;
in vec2 vUV;
in vec2 vUV1;
in vec4 vColor;
in float vDet;

// Material
uniform vec4 uMaterialColor;
uniform vec4 uLayerColor;
uniform int  uUseTexture;
uniform int  uUseVertexColor;
uniform sampler2D uTexture0;

// Shading: 0=Solid, 1=Matcap, 2=Textured (material preview)
uniform int uShadingMode;
uniform int uWireframeOverride;
uniform vec4 uWireColor;

// Matcap texture
uniform sampler2D uMatcap;

// Environment map (layer 1 blending, like Go project)
uniform int uUseEnvmap;
uniform sampler2D uEnvmap;

// Lighting
uniform vec3 uLightDir;
uniform vec3 uViewPos;

out vec4 FragColor;

void main() {
    if (uWireframeOverride == 1) {
        FragColor = uWireColor;
        return;
    }

    vec3 N = normalize(vWorldNormal);

    // ── Matcap Mode ──────────────────────────────────────────────────
    if (uShadingMode == 1) {
        vec3 vn = normalize(vViewNormal);
        vec2 matcapUV = vn.xy * 0.5 + 0.5;
        vec3 mc = texture(uMatcap, matcapUV).rgb;
        FragColor = vec4(mc, 1.0);
        return;
    }

    // ── Build base color ─────────────────────────────────────────────
    vec4 clr = vec4(1.0);

    if (uUseTexture == 1) {
        clr = texture(uTexture0, vUV);
    }

    // Environment map blending (Go-style: lerp diffuse→envmap by diffuse alpha)
    if (uUseEnvmap == 1) {
        vec3 envColor = texture(uEnvmap, vUV1).rgb;
        clr.rgb = clr.rgb * (1.0 - clr.a) + envColor * clr.a;
        clr.a = 1.0;
    }

    // Vertex color modulation
    if (uUseVertexColor == 1) {
        clr *= vColor;
    }

    // Material + layer tint
    clr *= uMaterialColor * uLayerColor;

    // Alpha test
    if (clr.a < 0.01) discard;

    // ── Textured Mode (material preview) ─────────────────────────────
    if (uShadingMode == 2) {
        float ndotl = max(dot(N, normalize(uLightDir)), 0.0);
        vec3 lighting = vec3(0.30) + vec3(0.70) * ndotl;
        FragColor = vec4(clr.rgb * lighting, clr.a);
        return;
    }

    // ── Solid Mode (Blinn-Phong with 3-point lighting) ───────────────
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);

    // Key light
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);

    // Fill light (soft, from opposite side)
    vec3 fillDir = normalize(vec3(-0.5, 0.3, -0.5));
    float fillDiff = max(dot(N, fillDir), 0.0);

    // Rim light (fresnel-based)
    float rim = 1.0 - max(dot(N, V), 0.0);
    rim = pow(rim, 3.0) * 0.15;

    vec3 ambient  = vec3(0.12);
    vec3 keyLight = vec3(1.0) * diff;
    vec3 specular = vec3(0.3) * spec;
    vec3 fill     = vec3(0.08, 0.10, 0.14) * fillDiff;

    vec3 lighting = ambient + keyLight + specular + fill + vec3(rim);
    FragColor = vec4(clr.rgb * lighting, clr.a);
}
)";

// ── Grid ────────────────────────────────────────────────────────────────

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

uniform vec4 uGridColor;

void main() {
    float fade = 1.0 - smoothstep(50000.0, 100000.0, vDist);
    bool isAxis = vColor.a > 0.6; 
    if (isAxis) {
        FragColor = vec4(vColor.rgb, vColor.a * fade);
    } else {
        FragColor = vec4(uGridColor.rgb, uGridColor.a * fade);
    }
}
)";

// ── Outline (backface extrusion with skinning support) ─────────────────

static const char* OUTLINE_VERT = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=5) in vec4 aBoneWeights;
layout(location=6) in uvec4 aBoneIndices;

uniform mat4 uModelTransform;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uOutlineThickness;
uniform mat4 uJoints[150];
uniform int  uUseJoints;

void main() {
    vec4 localPos;
    vec3 localNormal;

    if (uUseJoints == 1) {
        if ((aBoneWeights.x + aBoneWeights.y) > 0.001) {
            mat4 skin = uJoints[aBoneIndices.x] * aBoneWeights.x
                      + uJoints[aBoneIndices.y] * aBoneWeights.y;
            localPos    = skin * vec4(aPos, 1.0);
            localNormal = mat3(skin) * aNormal;
        } else {
            localPos    = uJoints[0] * vec4(aPos, 1.0);
            localNormal = mat3(uJoints[0]) * aNormal;
        }
    } else {
        localPos    = vec4(aPos, 1.0);
        localNormal = aNormal;
    }

    // Always apply model transform (handles Z-flip)
    vec4 worldPos   = uModelTransform * localPos;
    vec3 worldNormal = mat3(uModelTransform) * localNormal;

    // Extrude along world-space normal
    worldPos.xyz += normalize(worldNormal) * uOutlineThickness;
    gl_Position = uProjection * uView * worldPos;
}
)";

static const char* OUTLINE_FRAG = R"(
#version 330 core
uniform vec4 uOutlineColor;
out vec4 FragColor;

void main() {
    FragColor = uOutlineColor;
}
)";

// ── Background Gradient ────────────────────────────────────────────────
// Fullscreen triangle — no VBO needed, uses gl_VertexID.

static const char* BG_VERT = R"(
#version 330 core
out vec2 vUV;

void main() {
    // Generate fullscreen triangle from vertex ID (0,1,2)
    vUV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.999, 1.0);
}
)";

static const char* BG_FRAG = R"(
#version 330 core
in vec2 vUV;
uniform vec3 uTopColor;
uniform vec3 uBottomColor;
out vec4 FragColor;

void main() {
    // Smooth gradient from bottom to top
    float t = vUV.y;
    t = t * t * (3.0 - 2.0 * t); // smoothstep-like curve
    FragColor = vec4(mix(uBottomColor, uTopColor, t), 1.0);
}
)";

// ═══════════════════════════════════════════════════════════════════════════
// ShaderManager Implementation
// ═══════════════════════════════════════════════════════════════════════════

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
            char log[1024];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
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
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        LOG_ERR("[Shader] Link error: %s", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void ShaderManager::GenerateMatcapTexture() {
    const int size = 512;
    std::vector<uint8_t> pixels(size * size * 4);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = (float)x / (float)(size - 1) * 2.0f - 1.0f;
            float v = (float)y / (float)(size - 1) * 2.0f - 1.0f;
            float r2 = u * u + v * v;

            int idx = (y * size + x) * 4;

            if (r2 > 1.0f) {
                // Outside sphere: match viewport background
                pixels[idx + 0] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
                pixels[idx + 3] = 0;
            } else {
                float z = std::sqrt(1.0f - r2);

                // Key light (top-right, forward)
                float lx = 0.35f, ly = 0.65f, lz = 0.67f;
                float ll = std::sqrt(lx*lx + ly*ly + lz*lz);
                lx /= ll; ly /= ll; lz /= ll;

                float ndotl = std::max(0.0f, u*lx + (-v)*ly + z*lz);

                // Specular (Blinn-Phong)
                float hx = lx, hy = ly, hz = lz + 1.0f;
                float hl = std::sqrt(hx*hx + hy*hy + hz*hz);
                hx /= hl; hy /= hl; hz /= hl;
                float ndoth = std::max(0.0f, u*hx + (-v)*hy + z*hz);
                float spec = std::pow(ndoth, 80.0f);

                // Fill light (from left-bottom)
                float fx = -0.4f, fy = -0.3f, fz = 0.6f;
                float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
                fx /= fl; fy /= fl; fz /= fl;
                float fillDot = std::max(0.0f, u*fx + (-v)*fy + z*fz);

                // Fresnel rim
                float rim = 1.0f - z;
                rim = rim * rim * rim * 0.35f;

                // Clay-like warm base (or from AppConfig)
                float baseR = 0.62f, baseG = 0.58f, baseB = 0.56f;
                auto* cfg = AppConfig::Get();
                if (cfg) {
                    baseR = cfg->matcapR;
                    baseG = cfg->matcapG;
                    baseB = cfg->matcapB;
                }

                float ambient = 0.12f;
                float cr = baseR * (ambient + ndotl * 0.65f + fillDot * 0.12f) + spec * 0.45f + rim * 0.30f;
                float cg = baseG * (ambient + ndotl * 0.65f + fillDot * 0.12f) + spec * 0.45f + rim * 0.25f;
                float cb = baseB * (ambient + ndotl * 0.65f + fillDot * 0.10f) + spec * 0.45f + rim * 0.28f;

                cr = std::min(1.0f, std::max(0.0f, cr));
                cg = std::min(1.0f, std::max(0.0f, cg));
                cb = std::min(1.0f, std::max(0.0f, cb));

                // Edge antialiasing
                float edge = 1.0f - std::sqrt(r2);
                float edgeFade = std::min(1.0f, edge * size * 0.04f);

                pixels[idx + 0] = (uint8_t)(cr * edgeFade * 255.0f);
                pixels[idx + 1] = (uint8_t)(cg * edgeFade * 255.0f);
                pixels[idx + 2] = (uint8_t)(cb * edgeFade * 255.0f);
                pixels[idx + 3] = 255;
            }
        }
    }

    if (m_matcapTexture) {
        glDeleteTextures(1, &m_matcapTexture);
        m_matcapTexture = 0;
    }

    glGenTextures(1, &m_matcapTexture);
    glBindTexture(GL_TEXTURE_2D, m_matcapTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    LOG_INFO("[ShaderManager] Generated %dx%d matcap texture (id=%u).", size, size, m_matcapTexture);
}

void ShaderManager::GenerateBackgroundVAO() {
    glGenVertexArrays(1, &m_backgroundVAO);
}

void ShaderManager::Initialize() {
    if (m_initialized) return;
    m_initialized = true;

    // Unified scene shader (Solid / Matcap / Textured modes + skinning)
    Shader sceneShader;
    sceneShader.id = CompileProgram(SCENE_VERT, SCENE_FRAG);
    m_shaders["scene"] = sceneShader;

    // Grid shader
    Shader gridShader;
    gridShader.id = CompileProgram(GRID_VERT, GRID_FRAG);
    m_shaders["grid"] = gridShader;

    // Outline shader (with skinning)
    Shader outlineShader;
    outlineShader.id = CompileProgram(OUTLINE_VERT, OUTLINE_FRAG);
    m_shaders["outline"] = outlineShader;

    // Background gradient shader
    Shader bgShader;
    bgShader.id = CompileProgram(BG_VERT, BG_FRAG);
    m_shaders["background"] = bgShader;

    GenerateMatcapTexture();
    GenerateBackgroundVAO();

    LOG_INFO("[ShaderManager] Initialized %zu shaders.", m_shaders.size());
}

Shader* ShaderManager::GetShader(const std::string& name) {
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? &it->second : nullptr;
}

} // namespace GOW
