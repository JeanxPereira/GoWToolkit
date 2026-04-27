#pragma once
#include "IDocumentContent.h"
#include "rendering/Camera.h"
#include "rendering/GridRenderer.h"
#include "rendering/SceneRenderer.h"
#include "rendering/ShaderManager.h"
#include "core/parsers/shared/MeshData.h"
#include "core/parsers/shared/TextureData.h"
#include "core/parsers/shared/SceneNode.h"
#include <string>
#include <vector>
#include <memory>
#include <imgui.h>

namespace GOW {

class Viewport3D : public IDocumentContent {
public:
    Viewport3D(const std::string& name);
    ~Viewport3D() override;

    std::string GetName() const override;
    void Draw() override;
    void DrawInspector(AppContext& ctx) override;

    // Load mesh data into the viewport (routes through SceneRenderer)
    void LoadFromMeshData(const MeshData& data, const std::vector<std::unique_ptr<TextureData>>& textures = {});
    void LoadScene(std::unique_ptr<SceneData> scene);
    void ClearScene();

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
    void HandleInput();

    std::string m_name;
    Camera m_camera;
    GridRenderer m_grid;

    // Unified scene renderer — all content goes through here
    std::unique_ptr<SceneRenderer> m_sceneRenderer;

    // Keep scene data around for animation access
    std::shared_ptr<SceneData> m_sceneData;

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
};

} // namespace GOW
