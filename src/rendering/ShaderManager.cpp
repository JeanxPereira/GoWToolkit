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
void Shader::SetVec2(const char* name, const glm::vec2& v) const {
    glUniform2fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v));
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
out vec3 vViewPos;
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
    vViewPos     = (uView * worldPos).xyz;

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
in vec3 vViewPos;
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
        vec3 N = normalize(vViewNormal);
        // Blender matcap_uv_compute: perspective-correct orthonormal basis
        vec3 I = normalize(-vViewPos);  // view-space incident vector (camera→fragment)
        float a = 1.0 / (1.0 + I.z);
        float b = -I.x * I.y * a;
        vec3 b1 = vec3(1.0 - I.x * I.x * a, b, -I.x);
        vec3 b2 = vec3(b, 1.0 - I.y * I.y * a, -I.y);
        vec2 matcapUV = vec2(dot(b1, N), dot(b2, N)) * 0.496 + 0.5;
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
out vec2 vUV;

void main() {
    // Fullscreen triangle
    vUV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)";

static const char* GRID_FRAG = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform vec4 uGridColor;
uniform vec3 uCameraPos;
uniform float uGridScale;
uniform mat4 uViewProj;
uniform mat4 uInvViewProj;

void main() {
    vec2 ndc = vUV * 2.0 - 1.0;
    
    // Unproject near and far points to get world-space ray
    vec4 nearPt = uInvViewProj * vec4(ndc, -1.0, 1.0);
    vec4 farPt  = uInvViewProj * vec4(ndc,  1.0, 1.0);
    
    vec3 rayOrigin = nearPt.xyz / nearPt.w;
    vec3 rayEnd    = farPt.xyz / farPt.w;
    vec3 rayDir    = normalize(rayEnd - rayOrigin);
    
    // Intersect with Y=0 plane
    if (abs(rayDir.y) < 0.0001) discard;
    
    float t = -rayOrigin.y / rayDir.y;
    if (t < 0.0) discard; // Intersection is behind camera
    
    vec3 worldPos = rayOrigin + t * rayDir;
    
    // Calculate accurate depth for occlusion
    vec4 clipPos = uViewProj * vec4(worldPos, 1.0);
    float ndcZ = clipPos.z / clipPos.w;
    if (ndcZ < -1.0 || ndcZ > 1.0) discard;
    gl_FragDepth = ndcZ * 0.5 + 0.5;

    // --- Grid Drawing Math ---
    vec2 coord = worldPos.xz;
    vec2 deriv = max(fwidth(coord), vec2(0.00001));

    vec2 gridCoord = coord / uGridScale;
    vec2 gridDeriv = deriv / uGridScale;
    vec2 gridDist = abs(fract(gridCoord - 0.5) - 0.5) / gridDeriv;
    float gridAlpha = 1.0 - min(min(gridDist.x, gridDist.y), 1.0);

    float xAxisAlpha = 1.0 - min(abs(coord.y) / (deriv.y * 1.5), 1.0);
    float zAxisAlpha = 1.0 - min(abs(coord.x) / (deriv.x * 1.5), 1.0);

    float lodFade = max(0.0, 1.0 - max(gridDeriv.x, gridDeriv.y)); 
    gridAlpha *= lodFade;

    vec4 color = uGridColor;
    color.a *= gridAlpha;

    if (xAxisAlpha > 0.0) {
        color = mix(color, vec4(0.8, 0.2, 0.2, 0.8), xAxisAlpha);
    }
    if (zAxisAlpha > 0.0) {
        color = mix(color, vec4(0.2, 0.4, 0.8, 0.8), zAxisAlpha);
    }

    if (color.a < 0.01) discard;

    float d = length(worldPos - uCameraPos);
    float fade = 1.0 - smoothstep(0.0, 50000.0, d);

    if (uCameraPos.y < 0.0) {
        fade *= smoothstep(-10.0, 0.0, uCameraPos.y);
    }

    color.a *= fade;
    FragColor = color;
}
)";

// ── Outline Mask Shader (renders selected mesh to texture mask) ───────

static const char* OUTLINE_MASK_VERT = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=5) in vec4 aBoneWeights;
layout(location=6) in uvec4 aBoneIndices;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModelTransform;
uniform mat4 uJoints[150];
uniform int  uUseJoints;

out vec2 vUV;

void main() {
    vec4 localPos;
    if (uUseJoints == 1) {
        float w0 = aBoneWeights.x;
        float w1 = aBoneWeights.y;
        if (w0 > 0.001 && w1 > 0.001) {
            mat4 skin = uJoints[aBoneIndices.x] * w0
                      + uJoints[aBoneIndices.y] * w1;
            localPos = skin * vec4(aPos, 1.0);
        } else if (w0 > 0.001) {
            localPos = uJoints[aBoneIndices.x] * vec4(aPos, 1.0);
        } else {
            localPos = uJoints[aBoneIndices.y] * vec4(aPos, 1.0);
        }
    } else {
        localPos = vec4(aPos, 1.0);
    }
    vec4 worldPos = uModelTransform * localPos;
    gl_Position = uProjection * uView * worldPos;
    vUV = aUV;
}
)";

static const char* OUTLINE_MASK_FRAG = R"(
#version 330 core
in vec2 vUV;

uniform int uUseTexture;
uniform sampler2D uTexture0;

out vec4 FragColor;

void main() {
    // Alpha-test: if the texture has transparency, discard transparent pixels
    // so the outline follows the actual silhouette (hair, leaves, etc.)
    if (uUseTexture == 1) {
        float a = texture(uTexture0, vUV).a;
        if (a < 0.5) discard;
    }
    FragColor = vec4(1.0);  // White mask
}
)";

// ── Outline Post-Process Shader (runs fullscreen edge-detection) ──────

static const char* OUTLINE_POST_VERT = R"(
#version 330 core
out vec2 vUV;
void main() {
    vUV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)";

static const char* OUTLINE_POST_FRAG = R"(
#version 330 core
in vec2 vUV;

uniform sampler2D uMaskTex;
uniform vec2 uTexelSize;       // 1.0 / viewport size
uniform vec4 uOutlineColor;    // (1.0, 0.5, 0.0, 1.0) Blender orange
uniform float uOutlineWidth;   // in pixels, default 1.8

out vec4 FragColor;

void main() {
    float center = texture(uMaskTex, vUV).r;

    // Sample 4 cardinal neighbors (Blender style: ±X, ±Y)
    float px = texture(uMaskTex, vUV + vec2( uTexelSize.x, 0.0)).r;
    float nx = texture(uMaskTex, vUV + vec2(-uTexelSize.x, 0.0)).r;
    float py = texture(uMaskTex, vUV + vec2(0.0,  uTexelSize.y)).r;
    float ny = texture(uMaskTex, vUV + vec2(0.0, -uTexelSize.y)).r;

    // For thick outlines (2px), also sample at 2x distance
    float px2 = texture(uMaskTex, vUV + vec2( 2.0 * uTexelSize.x, 0.0)).r;
    float nx2 = texture(uMaskTex, vUV + vec2(-2.0 * uTexelSize.x, 0.0)).r;
    float py2 = texture(uMaskTex, vUV + vec2(0.0,  2.0 * uTexelSize.y)).r;
    float ny2 = texture(uMaskTex, vUV + vec2(0.0, -2.0 * uTexelSize.y)).r;

    // Edge detection: if center is background but a neighbor is selected (or vice versa)
    // This creates the outline on BOTH sides of the boundary
    float neighbors1 = max(max(px, nx), max(py, ny));
    float neighbors2 = max(max(px2, nx2), max(py2, ny2));

    // Anti-aliased edge weight using smooth interpolation
    float edge1 = abs(center - neighbors1);
    float edge2 = abs(center - neighbors2) * 0.5;  // Outer ring is softer
    float edgeWeight = clamp(edge1 + edge2, 0.0, 1.0);

    if (edgeWeight < 0.01 && center < 0.5) {
        discard;  // No outline, no fill → skip
    }

    vec4 color = vec4(0.0);

    // Outline border
    if (edgeWeight > 0.01) {
        color = uOutlineColor * edgeWeight;
    }

    // Interior fill (Blender's selection glow: ~15% alpha fill)
    if (center > 0.5 && edgeWeight < 0.5) {
        vec4 fill = vec4(uOutlineColor.rgb, 0.15);
        color = mix(fill, color, edgeWeight);
    }

    FragColor = color;
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

    // Outline mask shader (renders selected mesh to texture mask)
    Shader outlineMaskShader;
    outlineMaskShader.id = CompileProgram(OUTLINE_MASK_VERT, OUTLINE_MASK_FRAG);
    m_shaders["outline_mask"] = outlineMaskShader;

    // Outline post-process shader (runs fullscreen edge-detection)
    Shader outlinePostShader;
    outlinePostShader.id = CompileProgram(OUTLINE_POST_VERT, OUTLINE_POST_FRAG);
    m_shaders["outline_post"] = outlinePostShader;

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
