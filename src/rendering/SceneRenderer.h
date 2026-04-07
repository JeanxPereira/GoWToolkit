#pragma once
#include "core/parsers/shared/SceneNode.h"
#include "ShaderManager.h"
#include "GpuMesh.h"
#include <vector>
#include <memory>
#include <string>

// Forward-declare GL types
using GLenum = unsigned int;

namespace GOW {

/// A single renderable batch: one mesh part + its resolved material + GL resources.
struct RenderBatch {
    std::string                 name;
    bool                        isVisible = true;
    bool                        isHighlighted = false;

    std::shared_ptr<GpuMesh>    gpuMesh;
    GLuint                      texture0 = 0;       // Diffuse texture
    GLuint                      texture1 = 0;       // Environment map / layer 1
    float                       materialColor[4] = {1,1,1,1};
    float                       layerColor[4]    = {1,1,1,1};
    float                       uvOffset[2]      = {0,0};
    BlendMode                   blendMode = BlendMode::Normal;
    uint32_t                    textureLayer = 0;
    std::vector<uint16_t>       jointMap;
    bool                        hasTexture = false;
    bool                        hasEnvmap  = false;
    bool                        hasSkeleton = false;
};

/// Owns the GPU representation of a SceneData and renders it.
/// Created once when a viewport loads, destroyed when cleared.
class SceneRenderer {
public:
    SceneRenderer() = default;
    ~SceneRenderer() { Clear(); }

    // Non-copyable, movable
    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;
    SceneRenderer(SceneRenderer&&) = default;
    SceneRenderer& operator=(SceneRenderer&&) = default;

    /// Build GPU resources from parsed SceneData
    void Build(const SceneData& scene);

    /// Build from simple MeshData + textures (no materials/skeleton)
    void BuildFromMeshData(const MeshData& data,
                           const std::vector<std::unique_ptr<TextureData>>& textures);

    /// Render all batches with the specified shading mode
    void Render(const glm::mat4& view, const glm::mat4& proj, ShadingMode mode);

    /// Render sky batches with rotation-only view matrix, then clear depth.
    /// Call this BEFORE Render() for the main scene.
    void RenderSky(const glm::mat4& view, const glm::mat4& proj, ShadingMode mode);

    /// Render gradient background (static — no instance state needed)
    static void RenderBackground(const glm::vec3& topColor, const glm::vec3& bottomColor);

    /// Render debug skeleton lines
    void RenderSkeleton(const glm::mat4& view, const glm::mat4& proj);

    void Clear();

    bool IsEmpty() const { return m_batches.empty(); }
    bool HasSkeleton() const { return m_skeleton != nullptr; }
    bool HasSky() const { return m_hasSky; }
    BoundingBox GetBounds() const { return m_bounds; }

    int GetTotalVertices() const;
    int GetTotalTriangles() const;

    std::vector<RenderBatch>& GetBatches() { return m_batches; }
    bool GetDebugDisableSkin() const { return m_debugDisableSkin; }
    void SetDebugDisableSkin(bool v) { m_debugDisableSkin = v; }
private:
    void ComputeJointPalette();
    std::vector<glm::mat4> BuildBatchPalette(const RenderBatch& batch) const;
    void RenderBatches(const std::vector<RenderBatch*>& batches,
                       Shader* shader, const glm::mat4& view, const glm::mat4& proj,
                       ShadingMode mode);
    GLuint UploadTexture(const TextureData& tex);

    std::vector<RenderBatch>            m_batches;
    std::vector<RenderBatch*>           m_opaqueBatches;
    std::vector<RenderBatch*>           m_additiveBatches;
    std::vector<RenderBatch*>           m_skyBatches;
    bool                                m_hasSky = false;

    std::shared_ptr<ObjectData>         m_skeleton;
    glm::mat4                           m_instanceTransform {1.0f};

    // Precomputed joint palette for current pose
    std::vector<glm::mat4>              m_jointPalette;
    // World-space joint positions (for skeleton debug draw, separate from skinning palette)
    std::vector<glm::vec3>              m_jointWorldPos;

    // Owned GL textures (cleaned up in Clear())
    std::vector<GLuint>                 m_ownedTextures;

    BoundingBox                         m_bounds;

    bool                                m_diagDone = false; // reset on Clear()
    bool                                m_debugDisableSkin = false; // debug toggle
};

} // namespace GOW
