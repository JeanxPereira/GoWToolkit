#pragma once
#include "IDocumentContent.h"
#include "rendering/Camera.h"
#include "rendering/GpuMesh.h"
#include "rendering/GridRenderer.h"
#include "core/parsers/shared/MeshData.h"
#include "core/parsers/shared/TextureData.h"
#include <string>
#include <vector>
#include <memory>

namespace GOW {

class Viewport3D : public IDocumentContent {
public:
    Viewport3D(const std::string& name);
    ~Viewport3D() override;

    std::string GetName() const override;
    void Draw() override;

    // Load mesh data into the viewport
    void AddMesh(std::shared_ptr<GpuMesh> mesh);
    void ClearMeshes();
    void LoadFromMeshData(const MeshData& data, const std::vector<std::unique_ptr<TextureData>>& textures = {});
    
    // DEBUG: Add a simple colored cube to the scene
    void AddTestCube();

    // Render settings
    bool showGrid     = true;
    bool wireframe    = false;
    bool showNormals  = false;

private:
    void InitFBO();
    void ResizeFBO(int width, int height);

    std::string m_name;
    Camera m_camera;
    GridRenderer m_grid;

    std::vector<std::shared_ptr<GpuMesh>> m_meshes;
    std::vector<unsigned int> m_glTextures; // Store GL texture IDs for cleanup

    // FBO state
    unsigned int m_msaaFbo = 0;
    unsigned int m_msaaColor = 0;
    unsigned int m_msaaRbo = 0;
    
    unsigned int m_fbo = 0;
    unsigned int m_colorTex = 0;
    
    int m_fboWidth = 0;
    int m_fboHeight = 0;
    int m_msaaSamples = 4;
};

} // namespace GOW
