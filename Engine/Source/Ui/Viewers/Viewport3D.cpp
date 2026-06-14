#include "Viewport3D.h"
#include "Rendering/ShaderManager.h"
#include "Core/Events.h"
#include <glad/glad.h>
#include <imgui.h>
#include "Core/AppConfig.h"
#include "Core/ThemeManager.h"
#include "Fonts/SFSymbols.h"
#include "Ui/CameraPanel.h"
#include "Ui/Widgets.h"
#include "Ui/AnimationTimeline.h"
#include "Ui/ActiveAnimation.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Onyx::Viewers {

Viewport3D::Viewport3D(const std::string& name) : m_name(name) {
    InitFBO();
}

Viewport3D::~Viewport3D() {
    // Tear down active-anim broker if we set ourselves as the publisher;
    // otherwise downstream panels (Anim Curves / Dopesheet) keep a dangling
    // pointer into a freed AnimationPlayer.
    if (m_sceneRenderer &&
        Onyx::App::GetActiveAnimationPlayer() == m_sceneRenderer->GetAnimPlayer()) {
        Onyx::App::SetActiveAnimationPlayer(nullptr);
    }
    ClearScene();

    if (m_msaaFbo) glDeleteFramebuffers(1, &m_msaaFbo);
    if (m_msaaColor) glDeleteTextures(1, &m_msaaColor);
    if (m_msaaRbo) glDeleteRenderbuffers(1, &m_msaaRbo);

    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_colorTex) glDeleteTextures(1, &m_colorTex);
}

std::string Viewport3D::GetName() const { return m_name; }

void Viewport3D::ClearScene() {
    if (m_sceneRenderer &&
        Onyx::App::GetActiveAnimationPlayer() == m_sceneRenderer->GetAnimPlayer()) {
        Onyx::App::SetActiveAnimationPlayer(nullptr);
    }
    m_sceneRenderer.reset();
    m_sceneData.reset();
    m_needsRedraw = true;
}

void Viewport3D::LoadFromMeshData(const Parsers::MeshData& data, const std::vector<std::unique_ptr<Parsers::TextureData>>& textures) {
    ClearScene();
    if (data.parts.empty()) return;

    m_sceneRenderer = std::make_unique<Rendering::SceneRenderer>();
    m_sceneRenderer->BuildFromMeshData(data, textures);
    m_camera.FocusOn(m_sceneRenderer->GetBounds());
    m_needsRedraw = true;
}

void Viewport3D::LoadScene(std::unique_ptr<Parsers::SceneData> scene) {
    ClearScene();
    if (!scene || scene->IsEmpty()) return;

    // Keep a shared copy for animation access
    m_sceneData = std::shared_ptr<Parsers::SceneData>(scene.release());
    
    if (m_sceneData->animations) {
        EventAnimationLoaded::post(m_sceneData->animations);
    }

    m_sceneRenderer = std::make_unique<Rendering::SceneRenderer>();
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

    Rendering::ShaderManager::Get().Initialize();
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

    // Reserve a strip at the bottom of the viewport for the animation
    // transport (only when a clip is loaded). Image render area shrinks
    // by that height so the bar lives directly under the 3D scene.
    Onyx::Rendering::AnimationPlayer* transportPlayer =
        m_sceneRenderer ? m_sceneRenderer->GetAnimPlayer() : nullptr;
    const bool hasTransport =
        transportPlayer && transportPlayer->GetCurrentActIndex() >= 0 &&
        transportPlayer->GetFrameCount() > 0;
    const float transportHeight = hasTransport ? 86.0f : 0.0f;

    ImVec2 viewSize(avail.x, std::max(50.0f, avail.y - transportHeight));

    ResizeFBO((int)viewSize.x, (int)viewSize.y);

    // ── Animation update (every frame, regardless of redraw) ─────────
    float currentTime = (float)ImGui::GetTime();
    float dt = (m_lastFrameTime > 0.0f) ? (currentTime - m_lastFrameTime) : 0.0f;
    m_lastFrameTime = currentTime;

    if (m_sceneRenderer && m_sceneRenderer->UpdateAnimation(dt)) {
        m_needsRedraw = true;
    }
    if (m_camera.UpdateAnimation(dt)) {
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
            Rendering::SceneRenderer::RenderBackground(
                glm::vec3(cfg->bgTopR, cfg->bgTopG, cfg->bgTopB),
                glm::vec3(cfg->bgBotR, cfg->bgBotG, cfg->bgBotB)
            );
        } else if (hasContent) {
            Rendering::SceneRenderer::RenderBackground(bgTopColor, bgBottomColor);
        } else if (cfg) {
            // Empty viewport: darker gradient based on configured top color
            Rendering::SceneRenderer::RenderBackground(
                glm::vec3(cfg->bgTopR * 0.8f, cfg->bgTopG * 0.8f, cfg->bgTopB * 0.8f),
                glm::vec3(cfg->bgBotR * 0.8f, cfg->bgBotG * 0.8f, cfg->bgBotB * 0.8f)
            );
        } else {
            // Empty viewport: darker gradient
            Rendering::SceneRenderer::RenderBackground(
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
            
            m_sceneRenderer->Render(view, proj, shadingMode, m_fboWidth, m_fboHeight);

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
            m_grid.Draw(view, proj, m_camera.GetPosition(), gridColor, 1.0f);
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
    ImGui::Image((void*)(intptr_t)m_colorTex, viewSize, uv0, uv1);

    m_viewportHovered = ImGui::IsItemHovered();
    const ImVec2 imageMin = ImGui::GetItemRectMin();
    const ImVec2 imageMax = ImGui::GetItemRectMax();

    // ── Axis gizmo overlay ──────────────────────────────────────────
    Rendering::CameraView snapTarget;
    if (m_axisGizmo.Draw(m_camera.GetViewRotation(), imageMin, imageMax, snapTarget)) {
        m_camera.SnapToView(snapTarget);
        m_needsRedraw = true;
    }

    // ── Input ───────────────────────��───────────────────────────────
    HandleInput();

    // ── Toolbar overlay ─────────────────────────────────────────────
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    DrawToolbar(avail, cursorPos);

    // ── Object list ──────────────────────────────────────────────────
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

    // ── Animation transport bar (only when a clip is loaded) ─────────
    if (hasTransport) {
        DrawTransportBar();
    }
}

void Viewport3D::HandleInput() {
    if (!m_viewportHovered) return;

    ImGuiIO& io = ImGui::GetIO();

    // Suppress camera mouse handling while the axis gizmo owns the cursor —
    // otherwise a click-to-snap would also fire an orbit on drag-off.
    const bool gizmoHot = m_axisGizmo.IsHovered();

    // Right-drag: orbit
    if (!gizmoHot &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Right) &&
        (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)) {
        m_camera.ProcessMouseDrag(io.MouseDelta.x, io.MouseDelta.y);
        m_needsRedraw = true;
    }
    // Middle-drag: pan
    if (!gizmoHot &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle) &&
        (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)) {
        m_camera.ProcessMousePan(io.MouseDelta.x, io.MouseDelta.y);
        m_needsRedraw = true;
    }
    // Scroll: zoom
    if (!gizmoHot && io.MouseWheel != 0.0f) {
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
                case Rendering::ShadingMode::Solid:        shadingMode = Rendering::ShadingMode::Matcap;       break;
                case Rendering::ShadingMode::Matcap:       shadingMode = Rendering::ShadingMode::Textured;     break;
                case Rendering::ShadingMode::Textured:     shadingMode = Rendering::ShadingMode::Wireframe;    break;
                case Rendering::ShadingMode::Wireframe:    shadingMode = Rendering::ShadingMode::TexturedWire; break;
                case Rendering::ShadingMode::TexturedWire: shadingMode = Rendering::ShadingMode::Solid;        break;
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

    ImGui::PushStyleColor(ImGuiCol_Button, Onyx::Theme::ToolbarButton());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Onyx::Theme::ToolbarButtonHover());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Onyx::Theme::ToolbarButtonActive());

    namespace W = Onyx::App::Widgets;

    // Shading cycle — icon is the cube; mode goes in the tooltip.
    const char* shadingLabel = nullptr;
    switch (shadingMode) {
        case Rendering::ShadingMode::Solid:        shadingLabel = "Solid";      break;
        case Rendering::ShadingMode::Matcap:       shadingLabel = "Matcap";     break;
        case Rendering::ShadingMode::Textured:     shadingLabel = "Textured";   break;
        case Rendering::ShadingMode::Wireframe:    shadingLabel = "Wire";       break;
        case Rendering::ShadingMode::TexturedWire: shadingLabel = "Wire (Tex)"; break;
    }
    char shadingTip[64];
    snprintf(shadingTip, sizeof(shadingTip), "Shading: %s [Z]", shadingLabel);
    {
        W::IconButtonOpts opts;
        opts.tooltip = shadingTip;
        if (W::IconButton("vp_shading", ICON_SF_CUBE, opts)) {
            switch (shadingMode) {
                case Rendering::ShadingMode::Solid:        shadingMode = Rendering::ShadingMode::Matcap;       break;
                case Rendering::ShadingMode::Matcap:       shadingMode = Rendering::ShadingMode::Textured;     break;
                case Rendering::ShadingMode::Textured:     shadingMode = Rendering::ShadingMode::Wireframe;    break;
                case Rendering::ShadingMode::Wireframe:    shadingMode = Rendering::ShadingMode::TexturedWire; break;
                case Rendering::ShadingMode::TexturedWire: shadingMode = Rendering::ShadingMode::Solid;        break;
            }
            m_needsRedraw = true;
        }
    }

    ImGui::SameLine();
    {
        W::IconButtonOpts opts;
        opts.tooltip  = "Toggle grid [G]";
        opts.selected = showGrid;
        if (W::IconButton("vp_grid", ICON_SF_SQUARE_GRID_3X3, opts)) {
            showGrid = !showGrid;
            m_needsRedraw = true;
        }
    }

    if (m_sceneRenderer && !m_sceneRenderer->IsEmpty()) {
        ImGui::SameLine();
        W::IconButtonOpts opts;
        opts.tooltip  = "Toggle object list";
        opts.selected = showObjectList;
        if (W::IconButton("vp_objlist", ICON_SF_LIST_BULLET, opts)) {
            showObjectList = !showObjectList;
        }
    }

    ImGui::SameLine();
    {
        W::IconButtonOpts opts;
        opts.tooltip = "Frame all [F]";
        if (W::IconButton("vp_focus", ICON_SF_VIEWFINDER, opts)) {
            if (m_sceneRenderer && !m_sceneRenderer->IsEmpty()) {
                m_camera.FocusOn(m_sceneRenderer->GetBounds());
            } else {
                m_camera.Reset();
            }
            m_needsRedraw = true;
        }
    }

    ImGui::SameLine();
    {
        Onyx::App::CameraPanel* panel = Onyx::App::CameraPanel::Get();
        const bool camOpen = panel ? panel->visible : false;
        W::IconButtonOpts opts;
        opts.tooltip  = "Camera settings";
        opts.selected = camOpen;
        if (W::IconButton("vp_cam", ICON_SF_CAMERA, opts)) {
            Onyx::App::CameraPanel::Toggle();
            // Surface the dock tab on open so the user sees it without
            // hunting through tab headers.
            if (panel && panel->visible) ImGui::SetWindowFocus("Camera");
        }
    }

    // Show Bones / Skin toggles
    if (m_sceneRenderer && m_sceneRenderer->HasSkeleton()) {
        ImGui::SameLine();
        {
            W::IconButtonOpts opts;
            opts.tooltip  = "Toggle bones";
            opts.selected = showBones;
            if (W::IconButton("vp_bones", ICON_SF_FIGURE_WALK, opts)) {
                showBones = !showBones;
                m_needsRedraw = true;
            }
        }
        ImGui::SameLine();
        {
            const bool noSkin = m_sceneRenderer->GetDebugDisableSkin();
            W::IconButtonOpts opts;
            opts.tooltip  = "Toggle skinning (debug)";
            opts.selected = !noSkin;
            if (W::IconButton("vp_skin", ICON_SF_PERSON, opts)) {
                m_sceneRenderer->SetDebugDisableSkin(!noSkin);
                m_needsRedraw = true;
            }
        }
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

void Viewport3D::DrawInspector() {
    ImGui::Text("Viewport Settings");
    ImGui::Separator();

    const char* shadingLabel = "Solid\0Matcap\0Textured\0Wireframe\0TexturedWire\0";
    int mode = (int)shadingMode;
    if (ImGui::Combo("Shading", &mode, shadingLabel)) {
        shadingMode = (Rendering::ShadingMode)mode;
        m_needsRedraw = true;
    }

    if (ImGui::Checkbox("Show Grid", &showGrid)) m_needsRedraw = true;
    if (m_sceneRenderer && m_sceneRenderer->HasSkeleton()) {
        if (ImGui::Checkbox("Show Bones", &showBones)) m_needsRedraw = true;
    }

    // Publish whichever player this viewport currently owns so cross-cutting
    // panels (Anim Curves, Dopesheet) can read its playhead.
    if (m_sceneRenderer) {
        Onyx::App::SetActiveAnimationPlayer(m_sceneRenderer->GetAnimPlayer());
    } else {
        Onyx::App::SetActiveAnimationPlayer(nullptr);
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

        // Transport now lives in the bar directly under the viewport.
        // The inspector keeps just the clip browser so users can pick
        // an act without leaving their docked panel.
        ImGui::TextDisabled("Transport controls below viewport ↓");
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

    auto renderVisibilityToggle = [this](Rendering::RenderBatch& batch) {
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

    auto renderHighlightOnHover = [this](Rendering::RenderBatch& batch) {
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

void Viewport3D::DrawTransportBar() {
    if (!m_sceneRenderer) return;
    Onyx::Rendering::AnimationPlayer* player = m_sceneRenderer->GetAnimPlayer();
    if (!player || player->GetCurrentActIndex() < 0) return;

    bool  isPlaying   = player->IsPlaying();
    float dur         = player->GetDuration();
    int   totalFrames = std::max(1, player->GetFrameCount());
    int   curFrame    = player->GetCurrentFrame();

    // Tinted background that visually separates the strip from the 3D image.
    ImVec4 bgCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(bgCol.x, bgCol.y, bgCol.z, 0.9f));
    ImGui::BeginChild("##transport", ImVec2(0, 0), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Row 1: transport buttons + speed + loop mode
    const float btnW = 28.0f;
    if (ImGui::Button("|<", ImVec2(btnW, 0))) { player->SetFrame(0); m_needsRedraw = true; }
    ImGui::SameLine();
    if (ImGui::Button("<",  ImVec2(btnW, 0))) { player->SetFrame(curFrame - 1); m_needsRedraw = true; }
    ImGui::SameLine();
    if (ImGui::Button(isPlaying ? "Pause" : "Play", ImVec2(60, 0))) {
        if (dur > 0.0f) { player->Toggle(); m_needsRedraw = true; }
    }
    ImGui::SameLine();
    if (ImGui::Button(">",  ImVec2(btnW, 0))) { player->SetFrame(curFrame + 1); m_needsRedraw = true; }
    ImGui::SameLine();
    if (ImGui::Button(">|", ImVec2(btnW, 0))) { player->SetFrame(totalFrames - 1); m_needsRedraw = true; }
    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(50, 0))) { m_sceneRenderer->StopAnimation(); m_needsRedraw = true; }

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
    ImGui::Text("F %d/%d  %.2fs/%.2fs", curFrame, totalFrames - 1, player->GetTime(), dur);

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
    ImGui::PushItemWidth(70);
    float speed = player->GetSpeed();
    if (ImGui::DragFloat("##speed", &speed, 0.05f, -4.0f, 4.0f, "%.2fx")) {
        player->SetSpeed(speed);
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();

    const struct { const char* lbl; float val; } presets[] = {
        {".25", 0.25f}, {".5", 0.5f}, {"1x", 1.0f}, {"2x", 2.0f}, {"-1x", -1.0f},
    };
    for (auto& p : presets) {
        if (ImGui::SmallButton(p.lbl)) player->SetSpeed(p.val);
        ImGui::SameLine();
    }

    ImGui::PushItemWidth(90);
    int loopMode = (int)player->GetLoopMode();
    const char* loopLabels[] = { "No Loop", "Loop", "PingPong" };
    if (ImGui::Combo("##loop", &loopMode, loopLabels, IM_ARRAYSIZE(loopLabels))) {
        player->SetLoopMode((Rendering::AnimationPlayer::LoopMode)loopMode);
    }
    ImGui::PopItemWidth();

    // Keyboard shortcuts: Space/arrows/Home/End. Active while the viewport
    // window has focus (covers both the image hover and the transport).
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
        m_viewportHovered) {
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            if (dur > 0.0f) { player->Toggle(); m_needsRedraw = true; }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
            player->SetFrame(player->GetCurrentFrame() - 1); m_needsRedraw = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
            player->SetFrame(player->GetCurrentFrame() + 1); m_needsRedraw = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) {
            player->SetFrame(0); m_needsRedraw = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_End, false)) {
            player->SetFrame(totalFrames - 1); m_needsRedraw = true;
        }
    }

    // Row 2: rich timeline with frame ticks, keyframe markers, scrub
    if (Onyx::App::DrawAnimationTimeline("anim_timeline", *player)) {
        m_needsRedraw = true;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace Onyx::Viewers
