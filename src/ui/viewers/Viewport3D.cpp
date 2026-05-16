#include "Viewport3D.h"
#include "rendering/ShaderManager.h"
#include "core/Events.h"
#include <glad/glad.h>
#include <imgui.h>
#include "core/AppConfig.h"
#include <glm/gtc/matrix_transform.hpp>

namespace GOW {

Viewport3D::Viewport3D(const std::string& name) : m_name(name) {
    InitFBO();
}

Viewport3D::~Viewport3D() {
    ClearScene();

    if (m_msaaFbo) glDeleteFramebuffers(1, &m_msaaFbo);
    if (m_msaaColor) glDeleteTextures(1, &m_msaaColor);
    if (m_msaaRbo) glDeleteRenderbuffers(1, &m_msaaRbo);

    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_colorTex) glDeleteTextures(1, &m_colorTex);
}

std::string Viewport3D::GetName() const { return m_name; }

void Viewport3D::ClearScene() {
    m_sceneRenderer.reset();
    m_sceneData.reset();
    m_needsRedraw = true;
}

void Viewport3D::LoadFromMeshData(const MeshData& data, const std::vector<std::unique_ptr<TextureData>>& textures) {
    ClearScene();
    if (data.parts.empty()) return;

    m_sceneRenderer = std::make_unique<SceneRenderer>();
    m_sceneRenderer->BuildFromMeshData(data, textures);
    m_camera.FocusOn(m_sceneRenderer->GetBounds());
    m_needsRedraw = true;
}

void Viewport3D::LoadScene(std::unique_ptr<SceneData> scene) {
    ClearScene();
    if (!scene || scene->IsEmpty()) return;

    // Keep a shared copy for animation access
    m_sceneData = std::shared_ptr<SceneData>(scene.release());
    
    if (m_sceneData->animations) {
        EventAnimationLoaded::post(m_sceneData->animations);
    }

    m_sceneRenderer = std::make_unique<SceneRenderer>();
    m_sceneRenderer->Build(*m_sceneData);
    m_camera.FocusOn(m_sceneRenderer->GetBounds());
    m_needsRedraw = true;
    m_lastFrameTime = 0.0f;
}

void Viewport3D::InitFBO() {
    glGenFramebuffers(1, &m_msaaFbo);
    glGenTextures(1, &m_msaaColor);
    glGenRenderbuffers(1, &m_msaaRbo);

    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_colorTex);

    ShaderManager::Get().Initialize();
}

void Viewport3D::ResizeFBO(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == m_fboWidth && height == m_fboHeight) return;

    m_fboWidth = width;
    m_fboHeight = height;
    m_needsRedraw = true;

    // MSAA FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFbo);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_msaaColor);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaaSamples, GL_RGBA8, width, height, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_msaaColor, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, m_msaaRbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_msaaSamples, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msaaRbo);

    // Resolve FBO
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

    // ── Animation update (every frame, regardless of redraw) ─────────
    float currentTime = (float)ImGui::GetTime();
    float dt = (m_lastFrameTime > 0.0f) ? (currentTime - m_lastFrameTime) : 0.0f;
    m_lastFrameTime = currentTime;

    if (m_sceneRenderer && m_sceneRenderer->UpdateAnimation(dt)) {
        m_needsRedraw = true;
    }

    // ── Render to FBO ────────────────────────────────────────────────
    if (m_needsRedraw && m_fboWidth > 0 && m_fboHeight > 0) {
        m_needsRedraw = false;

        float aspect = (float)m_fboWidth / (float)m_fboHeight;
        glm::mat4 view = m_camera.GetViewMatrix();
        glm::mat4 proj = m_camera.GetProjectionMatrix(aspect);

        glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFbo);
        glViewport(0, 0, m_fboWidth, m_fboHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);

        bool hasContent = m_sceneRenderer && !m_sceneRenderer->IsEmpty();
        auto* cfg = AppConfig::Get();

        // ── Background gradient ────────────────────��─────────────────
        if (hasContent && cfg) {
            SceneRenderer::RenderBackground(
                glm::vec3(cfg->bgTopR, cfg->bgTopG, cfg->bgTopB),
                glm::vec3(cfg->bgBotR, cfg->bgBotG, cfg->bgBotB)
            );
        } else if (hasContent) {
            SceneRenderer::RenderBackground(bgTopColor, bgBottomColor);
        } else if (cfg) {
            // Empty viewport: darker gradient based on configured top color
            SceneRenderer::RenderBackground(
                glm::vec3(cfg->bgTopR * 0.8f, cfg->bgTopG * 0.8f, cfg->bgTopB * 0.8f),
                glm::vec3(cfg->bgBotR * 0.8f, cfg->bgBotG * 0.8f, cfg->bgBotB * 0.8f)
            );
        } else {
            // Empty viewport: darker gradient
            SceneRenderer::RenderBackground(
                glm::vec3(0.14f, 0.14f, 0.16f),
                glm::vec3(0.06f, 0.06f, 0.08f)
            );
        }

        // Re-enable depth after background pass (RenderBackground disables it)
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);

        // ── Scene ────────────────────────────────────────────────────
        if (hasContent) {
            if (m_sceneRenderer->HasSky()) {
                m_sceneRenderer->RenderSky(view, proj, shadingMode);
            }
            
            m_sceneRenderer->Render(view, proj, shadingMode);

            if (showBones && m_sceneRenderer->HasSkeleton()) {
                m_sceneRenderer->RenderSkeleton(view, proj);
            }
        }

        // ── Grid ────────────────────────────────────────────────────────────
        if (showGrid) {
            glm::vec4 gridColor = cfg ? glm::vec4(cfg->gridR, cfg->gridG, cfg->gridB, cfg->gridA) 
                                      : glm::vec4(0.35f, 0.35f, 0.35f, 0.5f);
            // Draw grid WITHOUT writing to the depth buffer so it doesn't clip transparent objects
            glDepthMask(GL_FALSE);
            // Allow grid lines to render if exactly coplanar
            glDepthFunc(GL_LEQUAL);
            m_grid.Draw(view, proj, gridColor);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
        }

        // ── Resolve MSAA ────────────────────────���────────────────────
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
        glBlitFramebuffer(0, 0, m_fboWidth, m_fboHeight, 0, 0, m_fboWidth, m_fboHeight,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_MULTISAMPLE);
    }

    // ── Display cached texture ────────────────────────��─────────────
    ImVec2 uv0(0, 1), uv1(1, 0); // flip Y for OpenGL
    ImGui::Image((void*)(intptr_t)m_colorTex, avail, uv0, uv1);

    m_viewportHovered = ImGui::IsItemHovered();

    // ── Input ───────────────────────��───────────────────────────────
    HandleInput();

    // ── Toolbar overlay ─────────────────────────────────────────────
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    DrawToolbar(avail, cursorPos);

    // ── Object list ────────────────────────────��────────────────────
    DrawObjectList(avail, cursorPos);

    // ── Empty viewport message ─────────────────────────���────────────
    if (!m_sceneRenderer || m_sceneRenderer->IsEmpty()) {
        const char* msg = "No mesh loaded";
        ImVec2 textSize = ImGui::CalcTextSize(msg);
        ImGui::SetCursorScreenPos(ImVec2(
            cursorPos.x + (avail.x - textSize.x) * 0.5f,
            cursorPos.y - avail.y * 0.5f - textSize.y * 0.5f
        ));
        ImGui::TextDisabled("%s", msg);
    }
}

void Viewport3D::HandleInput() {
    if (!m_viewportHovered) return;

    ImGuiIO& io = ImGui::GetIO();

    // Right-drag: orbit
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) &&
        (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)) {
        m_camera.ProcessMouseDrag(io.MouseDelta.x, io.MouseDelta.y);
        m_needsRedraw = true;
    }
    // Middle-drag: pan
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) &&
        (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)) {
        m_camera.ProcessMousePan(io.MouseDelta.x, io.MouseDelta.y);
        m_needsRedraw = true;
    }
    // Scroll: zoom
    if (io.MouseWheel != 0.0f) {
        m_camera.ProcessScroll(io.MouseWheel);
        m_needsRedraw = true;
    }

    // Keyboard shortcuts
    if (!io.WantCaptureKeyboard) {
        // F: Focus / frame all
        if (ImGui::IsKeyPressed(ImGuiKey_F) && m_sceneRenderer && !m_sceneRenderer->IsEmpty()) {
            m_camera.FocusOn(m_sceneRenderer->GetBounds());
            m_needsRedraw = true;
        }
        // Z: Cycle shading mode
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
            switch (shadingMode) {
                case ShadingMode::Solid:        shadingMode = ShadingMode::Matcap;       break;
                case ShadingMode::Matcap:       shadingMode = ShadingMode::Textured;     break;
                case ShadingMode::Textured:     shadingMode = ShadingMode::Wireframe;    break;
                case ShadingMode::Wireframe:    shadingMode = ShadingMode::TexturedWire; break;
                case ShadingMode::TexturedWire: shadingMode = ShadingMode::Solid;        break;
            }
            m_needsRedraw = true;
        }
        // G: Toggle grid
        if (ImGui::IsKeyPressed(ImGuiKey_G)) {
            showGrid = !showGrid;
            m_needsRedraw = true;
        }
        // Numpad 5: Reset camera
        if (ImGui::IsKeyPressed(ImGuiKey_Keypad5)) {
            m_camera.Reset();
            m_needsRedraw = true;
        }
    }
}

void Viewport3D::DrawToolbar(ImVec2 avail, ImVec2 cursorPos) {
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + 4, cursorPos.y - avail.y + 4));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.3f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.4f, 0.95f));

    // Shading mode cycle button
    const char* shadingLabel = nullptr;
    switch (shadingMode) {
        case ShadingMode::Solid:        shadingLabel = "Solid";     break;
        case ShadingMode::Matcap:       shadingLabel = "Matcap";    break;
        case ShadingMode::Textured:     shadingLabel = "Textured";  break;
        case ShadingMode::Wireframe:    shadingLabel = "Wire";      break;
        case ShadingMode::TexturedWire: shadingLabel = "Wire (Tex)";break;
    }
    if (ImGui::SmallButton(shadingLabel)) {
        switch (shadingMode) {
            case ShadingMode::Solid:        shadingMode = ShadingMode::Matcap;       break;
            case ShadingMode::Matcap:       shadingMode = ShadingMode::Textured;     break;
            case ShadingMode::Textured:     shadingMode = ShadingMode::Wireframe;    break;
            case ShadingMode::Wireframe:    shadingMode = ShadingMode::TexturedWire; break;
            case ShadingMode::TexturedWire: shadingMode = ShadingMode::Solid;        break;
        }
        m_needsRedraw = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shading mode [Z]");

    ImGui::SameLine();
    if (ImGui::SmallButton(showGrid ? "Grid" : "No Grid")) {
        showGrid = !showGrid;
        m_needsRedraw = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle grid [G]");

    if (m_sceneRenderer && !m_sceneRenderer->IsEmpty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton(showObjectList ? "List" : "No List")) {
            showObjectList = !showObjectList;
        }
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Focus")) {
        if (m_sceneRenderer && !m_sceneRenderer->IsEmpty()) {
            m_camera.FocusOn(m_sceneRenderer->GetBounds());
        } else {
            m_camera.Reset();
        }
        m_needsRedraw = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Frame all [F]");

    // Show Bones toggle
    if (m_sceneRenderer && m_sceneRenderer->HasSkeleton()) {
        ImGui::SameLine();
        if (ImGui::SmallButton(showBones ? "Bones" : "No Bones")) {
            showBones = !showBones;
            m_needsRedraw = true;
        }
        ImGui::SameLine();
        bool noSkin = m_sceneRenderer->GetDebugDisableSkin();
        if (ImGui::SmallButton(noSkin ? "[NoSkin]" : "Skin")) {
            m_sceneRenderer->SetDebugDisableSkin(!noSkin);
            m_needsRedraw = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle skinning (debug)");
    }

    // Stats
    if (m_sceneRenderer && !m_sceneRenderer->IsEmpty()) {
        int totalVerts = m_sceneRenderer->GetTotalVertices();
        int totalTris  = m_sceneRenderer->GetTotalTriangles();
        if (totalVerts > 0) {
            ImGui::SameLine();
            ImGui::TextDisabled("| %d verts, %d tris", totalVerts, totalTris);
        }
    }

    // FPS counter — bottom center
    {
        char fpsBuf[32];
        snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", ImGui::GetIO().Framerate);
        ImVec2 fpsSize = ImGui::CalcTextSize(fpsBuf);
        ImGui::SetCursorScreenPos(ImVec2(
            cursorPos.x + (avail.x - fpsSize.x) * 0.5f,
            cursorPos.y - fpsSize.y - 4.0f
        ));
        ImGui::TextDisabled("%s", fpsBuf);
    }

    ImGui::PopStyleColor(3);
}

void Viewport3D::DrawInspector(AppContext& ctx) {
    ImGui::Text("Viewport Settings");
    ImGui::Separator();

    const char* shadingLabel = "Solid\0Matcap\0Textured\0Wireframe\0TexturedWire\0";
    int mode = (int)shadingMode;
    if (ImGui::Combo("Shading", &mode, shadingLabel)) {
        shadingMode = (ShadingMode)mode;
        m_needsRedraw = true;
    }

    if (ImGui::Checkbox("Show Grid", &showGrid)) m_needsRedraw = true;
    if (m_sceneRenderer && m_sceneRenderer->HasSkeleton()) {
        if (ImGui::Checkbox("Show Bones", &showBones)) m_needsRedraw = true;
    }

    // ── Animation Section ─────────────────────────────────────────────
    if (m_sceneRenderer && m_sceneRenderer->HasAnimations()) {
        ImGui::Separator();
        ImGui::Text("Animations");

        auto* animData = m_sceneRenderer->GetAnimationData();
        auto* player = m_sceneRenderer->GetAnimPlayer();

        // Animation group/act tree
        ImGui::BeginChild("AnimTree", ImVec2(0, 150), true);
        for (int ig = 0; ig < (int)animData->groups.size(); ++ig) {
            const auto& group = animData->groups[ig];
            if (group.isExternal || group.acts.empty()) continue;

            // Only show skinning acts (type 0)
            int skinIdx = animData->FindSkinningTypeIndex();
            if (skinIdx < 0) continue;

            bool groupOpen = ImGui::TreeNodeEx(group.name.c_str(),
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

            if (groupOpen) {
                for (int ia = 0; ia < (int)group.acts.size(); ++ia) {
                    const auto& act = group.acts[ia];

                    bool isSelected = player && player->IsPlaying()
                        && player->GetCurrentGroupIndex() == ig
                        && player->GetCurrentActIndex() == ia;

                    char label[128];
                    snprintf(label, sizeof(label), "%s  [%.1fs]",
                             act.name.c_str(), act.duration);

                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
                        | ImGuiTreeNodeFlags_NoTreePushOnOpen
                        | ImGuiTreeNodeFlags_SpanAvailWidth;
                    if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

                    ImGui::TreeNodeEx((void*)(intptr_t)(ig * 1000 + ia), flags, "%s", label);

                    // Double-click to play
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        m_sceneRenderer->SetAnimation(ig, ia);
                        m_needsRedraw = true;
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();

        // Transport controls
        bool isPlaying = player && player->IsPlaying();

        if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(60, 0))) {
            if (isPlaying) {
                m_sceneRenderer->StopAnimation();
                m_needsRedraw = true;
            } else if (player) {
                // Resume with current group/act
                int g = player->GetCurrentGroupIndex();
                int a = player->GetCurrentActIndex();
                if (g >= 0 && a >= 0) {
                    m_sceneRenderer->SetAnimation(g, a);
                    m_needsRedraw = true;
                }
            }
        }

        if (player) {
            ImGui::SameLine();
            bool loop = player->IsLooping();
            if (ImGui::Checkbox("Loop", &loop)) {
                player->SetLooping(loop);
            }

            // Timeline slider
            if (isPlaying || player->GetDuration() > 0.0f) {
                float t = player->GetTime();
                float dur = player->GetDuration();
                if (dur > 0.0f) {
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::SliderFloat("##timeline", &t, 0.0f, dur, "%.2fs / %.2fs")) {
                        player->SetTime(t);
                        m_needsRedraw = true;
                    }
                }
            }

            // Current animation name
            if (isPlaying) {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Playing: %s",
                                 player->GetCurrentActName().c_str());
            }
        }
    }

    // ── Mesh Batches ────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("Scene Mesh Batches");

    if (!m_sceneRenderer || m_sceneRenderer->IsEmpty()) {
        ImGui::TextDisabled("No meshes in scene.");
        return;
    }

    auto& batches = m_sceneRenderer->GetBatches();

    // Group consecutive batches that share the same non-zero meshHash:
    //   each shared LOD blob in the lodpack is referenced by N consecutive
    //   submeshes representing LOD0..LODn of one logical mesh part.
    //   meshHash == 0 (internal/embedded LOD) gets its own single-entry group.
    struct LodGroup { uint64_t hash; std::vector<size_t> idx; };
    std::vector<LodGroup> groups;
    for (size_t i = 0; i < batches.size(); ++i) {
        const uint64_t h = batches[i].meshHash;
        if (h != 0 && !groups.empty() && groups.back().hash == h) {
            groups.back().idx.push_back(i);
        } else {
            groups.push_back({h, {i}});
        }
    }

    auto renderVisibilityToggle = [this](RenderBatch& batch) {
        bool prev = batch.isVisible;
        ImGui::Checkbox("##vis", &batch.isVisible);
        bool itemHovered = ImGui::IsItemHovered();
        bool toggledByClick = (prev != batch.isVisible);

        if (toggledByClick) {
            m_dragToggleActive = true;
            m_dragToggleValue  = batch.isVisible;
            m_needsRedraw = true;
        } else if (m_dragToggleActive && itemHovered &&
                   ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (batch.isVisible != m_dragToggleValue) {
                batch.isVisible = m_dragToggleValue;
                m_needsRedraw = true;
            }
        }
    };

    auto renderHighlightOnHover = [this](RenderBatch& batch) {
        bool hovered = ImGui::IsItemHovered();
        if (hovered != batch.isHighlighted) {
            batch.isHighlighted = hovered;
            m_needsRedraw = true;
        }
    };

    ImGui::BeginChild("MeshBatches", ImVec2(0, 0), true);

    for (size_t g = 0; g < groups.size(); ++g) {
        const auto& grp = groups[g];

        // Single-entry group (internal LOD or lone hashed part): render flat row
        if (grp.idx.size() == 1) {
            size_t i = grp.idx[0];
            auto& batch = batches[i];
            ImGui::PushID((int)i);
            renderVisibilityToggle(batch);
            ImGui::SameLine();
            std::string label = batch.name.empty() ? ("Part " + std::to_string(i)) : batch.name;
            if (batch.meshHash == 0) label += "  (internal)";
            ImGui::Selectable(label.c_str(), false);
            renderHighlightOnHover(batch);
            ImGui::PopID();
            continue;
        }

        // Multi-LOD group: collapsible tree
        ImGui::PushID((int)(1000 + g));

        // Group-level visibility checkbox: ANY visible → checked; toggling sets all
        bool anyVisible = false, allVisible = true;
        for (size_t i : grp.idx) {
            if (batches[i].isVisible) anyVisible = true;
            else                       allVisible = false;
        }
        bool groupVis = anyVisible;
        bool prevGroupVis = groupVis;
        ImGui::Checkbox("##groupvis", &groupVis);
        bool groupHovered = ImGui::IsItemHovered();
        bool groupClicked = (prevGroupVis != groupVis);

        if (groupClicked) {
            for (size_t i : grp.idx) batches[i].isVisible = groupVis;
            m_dragToggleActive = true;
            m_dragToggleValue  = groupVis;
            m_needsRedraw = true;
        } else if (m_dragToggleActive && groupHovered &&
                   ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            bool wantVis = m_dragToggleValue;
            if (anyVisible != wantVis || !allVisible) {
                for (size_t i : grp.idx) batches[i].isVisible = wantVis;
                m_needsRedraw = true;
            }
        }

        ImGui::SameLine();

        char header[96];
        std::snprintf(header, sizeof(header),
                      "Mesh Group %zu  (%zu LODs, hash %016llX)",
                      g, grp.idx.size(),
                      (unsigned long long)grp.hash);

        bool open = ImGui::TreeNodeEx(header,
            ImGuiTreeNodeFlags_SpanAvailWidth);

        if (open) {
            for (size_t k = 0; k < grp.idx.size(); ++k) {
                size_t i = grp.idx[k];
                auto& batch = batches[i];
                ImGui::PushID((int)i);
                renderVisibilityToggle(batch);
                ImGui::SameLine();
                char lodLabel[96];
                std::snprintf(lodLabel, sizeof(lodLabel),
                              "LOD %zu  (%dv, %dt)",
                              k, batch.vertexCount, batch.triangleCount);
                ImGui::Selectable(lodLabel, false);
                renderHighlightOnHover(batch);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    // Release the drag-toggle smear on mouse-up
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        m_dragToggleActive = false;
    }
}

void Viewport3D::DrawObjectList(ImVec2 avail, ImVec2 cursorPos) {
    // Moved to DrawInspector
}

} // namespace GOW
