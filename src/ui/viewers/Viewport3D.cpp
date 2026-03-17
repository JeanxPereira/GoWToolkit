#include "Viewport3D.h"
#include "rendering/ShaderManager.h"
#include <glad/glad.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

namespace GOW {

Viewport3D::Viewport3D(const std::string& name) : m_name(name) {
    InitFBO();
}

Viewport3D::~Viewport3D() {
    ClearMeshes(); // This will delete m_glTextures and clear m_meshes

    if (m_msaaFbo) glDeleteFramebuffers(1, &m_msaaFbo);
    if (m_msaaColor) glDeleteTextures(1, &m_msaaColor);
    if (m_msaaRbo) glDeleteRenderbuffers(1, &m_msaaRbo);
    
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_colorTex) glDeleteTextures(1, &m_colorTex);
}

std::string Viewport3D::GetName() const { return m_name; }

void Viewport3D::AddMesh(std::shared_ptr<GpuMesh> mesh) {
    m_meshes.push_back(mesh);
    // Auto-focus on first mesh
    if (m_meshes.size() == 1) {
        m_camera.FocusOn(mesh->GetBounds());
    }
}

void Viewport3D::ClearMeshes() {
    m_meshes.clear();
    if (!m_glTextures.empty()) {
        glDeleteTextures((GLsizei)m_glTextures.size(), m_glTextures.data());
        m_glTextures.clear();
    }
}

void Viewport3D::LoadFromMeshData(const MeshData& data, const std::vector<std::unique_ptr<TextureData>>& textures) {
    ClearMeshes();

    // 1. Upload all valid textures to OpenGL
    std::vector<GLuint> texMap(textures.size(), 0);
    for (size_t i = 0; i < textures.size(); i++) {
        const auto& tex = textures[i];
        if (tex && tex->IsValid()) {
            GLuint glTex;
            glGenTextures(1, &glTex);
            glBindTexture(GL_TEXTURE_2D, glTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->width, tex->height,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, tex->pixels.data());
            
            texMap[i] = glTex;
            m_glTextures.push_back(glTex);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // 2. Create meshes and assign textures
    for (const auto& part : data.parts) {
        if (part.indices.empty() || part.vertices.empty()) continue;
        auto gpuMesh = std::make_shared<GpuMesh>();
        gpuMesh->Upload(part.vertices, part.indices);

        if (part.materialId >= 0 && part.materialId < (int)texMap.size() && texMap[part.materialId] != 0) {
            gpuMesh->SetTexture(texMap[part.materialId]);
        }

        AddMesh(gpuMesh);
    }
}

void Viewport3D::AddTestCube() {
    auto cube = std::make_shared<GpuMesh>();
    
    std::vector<GpuVertex> verts = {
        // Front face (red)
        { {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f} },
        { { 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f} },
        { { 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.2f, 0.2f, 1.0f} },
        { {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.2f, 0.2f, 1.0f} },
        // Back face (green)
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f,-1.0f}, {0.0f, 0.0f}, {0.2f, 1.0f, 0.2f, 1.0f} },
        { {-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f,-1.0f}, {0.0f, 1.0f}, {0.2f, 1.0f, 0.2f, 1.0f} },
        { { 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f,-1.0f}, {1.0f, 1.0f}, {0.2f, 1.0f, 0.2f, 1.0f} },
        { { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f,-1.0f}, {1.0f, 0.0f}, {0.2f, 1.0f, 0.2f, 1.0f} },
        // Top face (blue)
        { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.2f, 0.2f, 1.0f, 1.0f} },
        { {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.2f, 0.2f, 1.0f, 1.0f} },
        { { 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.2f, 0.2f, 1.0f, 1.0f} },
        { { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.2f, 0.2f, 1.0f, 1.0f} },
        // Bottom face (yellow)
        { {-0.5f, -0.5f, -0.5f}, {0.0f,-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 0.2f, 1.0f} },
        { { 0.5f, -0.5f, -0.5f}, {0.0f,-1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 0.2f, 1.0f} },
        { { 0.5f, -0.5f,  0.5f}, {0.0f,-1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 0.2f, 1.0f} },
        { {-0.5f, -0.5f,  0.5f}, {0.0f,-1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 0.2f, 1.0f} },
        // Right face (magenta)
        { { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.2f, 1.0f, 1.0f} },
        { { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.2f, 1.0f, 1.0f} },
        { { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.2f, 1.0f, 1.0f} },
        { { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.2f, 1.0f, 1.0f} },
        // Left face (cyan)
        { {-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f},{0.0f, 0.0f}, {0.2f, 1.0f, 1.0f, 1.0f} },
        { {-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f},{1.0f, 0.0f}, {0.2f, 1.0f, 1.0f, 1.0f} },
        { {-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f},{1.0f, 1.0f}, {0.2f, 1.0f, 1.0f, 1.0f} },
        { {-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f},{0.0f, 1.0f}, {0.2f, 1.0f, 1.0f, 1.0f} }
    };
    
    std::vector<uint32_t> indices = {
        0, 1, 2,  2, 3, 0,       // Front
        4, 5, 6,  6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,     // Top
        12, 13, 14, 14, 15, 12,  // Bottom
        16, 17, 18, 18, 19, 16,  // Right
        20, 21, 22, 22, 23, 20   // Left
    };
    
    cube->Upload(verts, indices);
    AddMesh(cube);
}

void Viewport3D::InitFBO() {
    // Generate MSAA FBO
    glGenFramebuffers(1, &m_msaaFbo);
    glGenTextures(1, &m_msaaColor);
    glGenRenderbuffers(1, &m_msaaRbo);

    // Generate Resolve FBO
    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_colorTex);

    // Make sure shaders are ready
    ShaderManager::Get().Initialize();
}

void Viewport3D::ResizeFBO(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == m_fboWidth && height == m_fboHeight) return;

    m_fboWidth = width;
    m_fboHeight = height;

    // 1. Setup MSAA FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFbo);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_msaaColor);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaaSamples, GL_RGBA8, width, height, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_msaaColor, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, m_msaaRbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_msaaSamples, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msaaRbo);

    // 2. Setup Resolve FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Viewport3D::Draw() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x <= 0 || avail.y <= 0) return;

    ResizeFBO((int)avail.x, (int)avail.y);

    float aspect = (float)m_fboWidth / (float)m_fboHeight;
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::mat4 proj = m_camera.GetProjectionMatrix(aspect);
    glm::mat4 model = glm::mat4(1.0f);

    // ── Render to MSAA FBO ─────────────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFbo);
    glViewport(0, 0, m_fboWidth, m_fboHeight);
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Grid
    if (showGrid) {
        glDepthMask(GL_FALSE);
        m_grid.Draw(view, proj);
        glDepthMask(GL_TRUE);
    }

    // Meshes
    if (!m_meshes.empty()) {
        auto* shader = ShaderManager::Get().GetShader("default");
        if (shader) {
            shader->Use();
            shader->SetMat4("uModel", model);
            shader->SetMat4("uView", view);
            shader->SetMat4("uProjection", proj);
            shader->SetVec3("uLightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f)));
            shader->SetVec3("uViewPos", glm::vec3(glm::inverse(view)[3]));

            if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            for (auto& mesh : m_meshes) {
                if (mesh->HasTexture()) {
                    shader->SetInt("uUseTexture", 1);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh->GetTexture());
                    shader->SetInt("uTexture0", 0);
                } else {
                    shader->SetInt("uUseTexture", 0);
                }
                mesh->Draw();
            }

            // Unbind texture
            glBindTexture(GL_TEXTURE_2D, 0);

            if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    // ── Resolve MSAA FBO to Normal FBO ─────────────────────────────────
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
    glBlitFramebuffer(0, 0, m_fboWidth, m_fboHeight, 0, 0, m_fboWidth, m_fboHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_MULTISAMPLE);

    // ── Display in ImGui ───────────────────────────────────────────────
    ImVec2 uv0(0, 1), uv1(1, 0); // flip Y for OpenGL
    ImGui::Image((void*)(intptr_t)m_colorTex, avail, uv0, uv1);

    // ── Input handling ─────────────────────────────────────────────────
    if (ImGui::IsItemHovered()) {
        ImGuiIO& io = ImGui::GetIO();

        // Right-drag: orbit
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            m_camera.ProcessMouseDrag(io.MouseDelta.x, io.MouseDelta.y);
        }
        // Middle-drag: pan
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            m_camera.ProcessMousePan(io.MouseDelta.x, io.MouseDelta.y);
        }
        // Scroll: zoom
        if (io.MouseWheel != 0.0f) {
            m_camera.ProcessScroll(io.MouseWheel);
        }
    }

    // ── Toolbar overlay ────────────────────────────────────────────────
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + 4, cursorPos.y - avail.y + 4));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.3f, 0.9f));

    if (ImGui::SmallButton(wireframe ? "[W] Wire" : "[W] Solid")) wireframe = !wireframe;
    ImGui::SameLine();
    if (ImGui::SmallButton(showGrid ? "[G] Grid" : "[G] No Grid")) showGrid = !showGrid;
    ImGui::SameLine();
    if (ImGui::SmallButton("[R] Reset")) m_camera.Reset();

    // Stats
    int totalVerts = 0, totalTris = 0;
    for (auto& m : m_meshes) {
        totalVerts += m->GetVertexCount();
        totalTris  += m->GetIndexCount() / 3;
    }
    if (totalVerts > 0) {
        ImGui::SameLine();
        ImGui::TextDisabled("| %d verts, %d tris", totalVerts, totalTris);
    }

    ImGui::PopStyleColor(2);
}

} // namespace GOW
