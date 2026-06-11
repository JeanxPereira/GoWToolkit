#pragma once
#include "IDocumentContent.h"
#include "Rendering/Camera.h"
#include "Rendering/GridRenderer.h"
#include "Rendering/SceneRenderer.h"
#include "Rendering/ShaderManager.h"
#include "Rendering/AxisGizmo.h"
#include "Core/Parsers/Shared/MeshData.h"
#include "Core/Parsers/Shared/TextureData.h"
#include "Core/Parsers/Shared/SceneNode.h"
#include <string>
#include <vector>
#include <memory>
#include <imgui.h>

namespace Onyx {

class Viewport3D : public IDocumentContent {
public:
    Viewport3D(const std::string& name);
    ~Viewport3D() override;

    std::string GetName() const override;
    void Draw() override;
    void DrawInspector() override;
    Viewport3D* GetEmbeddedViewport() override { return this; }

    // Load mesh data into the viewport (routes through SceneRenderer)
    void LoadFromMeshData(const Parsers::MeshData& data, const std::vector<std::unique_ptr<Parsers::TextureData>>& textures = {});
    void LoadScene(std::unique_ptr<Parsers::SceneData> scene);
    void ClearScene();

    // Accessors for the CameraPanel (renders camera tuning UI as a dock tab
    // next to Inspector).
    Camera&            GetCamera()  { return m_camera; }
    SceneRenderer*     GetSceneRenderer() const { return m_sceneRenderer.get(); }
    int                GetFboWidth()  const { return m_fboWidth; }
    int                GetFboHeight() const { return m_fboHeight; }
    void               RequestRedraw()       { m_needsRedraw = true; }

    // Render settings
    bool showGrid       = true;
    bool showOutline    = true;
    bool showBones      = false;
    bool showObjectList = true;
    ShadingMode shadingMode = ShadingMode::Solid;

    // Outline settings
    glm::vec4 outlineColor      {0.0f, 0.0f, 0.0f, 1.0f};
    float     outlineThickness  = 0.015f;

    // Background gradient
    glm::vec3 bgTopColor    {0.18f, 0.18f, 0.22f};
    glm::vec3 bgBottomColor {0.08f, 0.08f, 0.10f};

private:
    void InitFBO();
    void ResizeFBO(int width, int height);
    void DrawToolbar(ImVec2 avail, ImVec2 cursorPos);
    void DrawObjectList(ImVec2 avail, ImVec2 cursorPos);
    void DrawTransportBar();  // bottom strip with anim transport + timeline
    void HandleInput();

    std::string m_name;
    Camera m_camera;
    GridRenderer m_grid;
    AxisGizmo m_axisGizmo;

    // Unified scene renderer — all content goes through here
    std::unique_ptr<SceneRenderer> m_sceneRenderer;

    // Keep scene data around for animation access
    std::shared_ptr<Parsers::SceneData> m_sceneData;

    // FBO state
    unsigned int m_msaaFbo = 0;
    unsigned int m_msaaColor = 0;
    unsigned int m_msaaRbo = 0;

    unsigned int m_fbo = 0;
    unsigned int m_colorTex = 0;

    int m_fboWidth = 0;
    int m_fboHeight = 0;
    int m_msaaSamples = 4;

    bool m_needsRedraw = true;
    bool m_viewportHovered = false;
    float m_lastFrameTime = 0.0f;  // For animation delta time

    // Photoshop-style click-and-drag toggling across visibility checkboxes
    bool m_dragToggleActive = false;
    bool m_dragToggleValue  = false;
};

} // namespace Onyx
