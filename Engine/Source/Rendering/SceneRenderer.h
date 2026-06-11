#pragma once
#include "Core/Parsers/Shared/SceneNode.h"
#include "ShaderManager.h"
#include "AnimationPlayer.h"
#include "GpuMesh.h"
#include <vector>
#include <memory>
#include <string>

// Forward-declare GL types
using GLenum = unsigned int;

namespace Onyx {

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
    Parsers::BlendMode          blendMode = Parsers::BlendMode::Normal;
    uint32_t                    textureLayer = 0;
    std::vector<uint16_t>       jointMap;
    bool                        hasTexture = false;
    bool                        hasEnvmap  = false;
    bool                        hasSkeleton = false;
    bool                        isSky = false;
    uint64_t                    meshHash = 0;       // GOWR LOD-blob id (0 = internal/embedded)
    int                         vertexCount = 0;    // cached for inspector
    int                         triangleCount = 0;
};

/// Owns the GPU representation of a Parsers::SceneData and renders it.
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

    /// Build GPU resources from parsed Parsers::SceneData
    void Build(const Parsers::SceneData& scene);

    /// Build from simple Parsers::MeshData + textures (no materials/skeleton)
    void BuildFromMeshData(const Parsers::MeshData& data,
                           const std::vector<std::unique_ptr<Parsers::TextureData>>& textures);

    /// Render all batches with the specified shading mode
    void Render(const glm::mat4& view, const glm::mat4& proj, ShadingMode mode, int viewportW, int viewportH);

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
    Domain::BoundingBox GetBounds() const { return m_bounds; }

    int GetTotalVertices() const;
    int GetTotalTriangles() const;

    std::vector<RenderBatch>& GetBatches() { return m_batches; }
    bool GetDebugDisableSkin() const { return m_debugDisableSkin; }
    void SetDebugDisableSkin(bool v) { m_debugDisableSkin = v; }

    // ── Animation API ──────────────────────────────────────────────────
    bool HasAnimations() const { return m_animData != nullptr; }
    const Parsers::AnimationData* GetAnimationData() const { return m_animData.get(); }
    AnimationPlayer* GetAnimPlayer() { return m_animPlayer.get(); }

    /// Start playing a specific animation group/act.
    void SetAnimation(int groupIdx, int actIdx);

    /// Stop and reset to idle pose.
    void StopAnimation();

    /// Advance animation. Returns true if the scene needs redrawing.
    bool UpdateAnimation(float dt);
private:
    void ComputeJointPalette();
    std::vector<glm::mat4> BuildBatchPalette(const RenderBatch& batch) const;
    void RenderBatches(const std::vector<RenderBatch*>& batches,
                       Shader* shader, const glm::mat4& view, const glm::mat4& proj,
                       ShadingMode mode);
    GLuint UploadTexture(const Parsers::TextureData& tex);

    std::vector<RenderBatch>            m_batches;
    std::vector<RenderBatch*>           m_opaqueBatches;
    std::vector<RenderBatch*>           m_additiveBatches;
    std::vector<RenderBatch*>           m_skyBatches;
    bool                                m_hasSky = false;

    std::shared_ptr<Parsers::ObjectData>         m_skeleton;
    glm::mat4                           m_instanceTransform {1.0f};

    // Precomputed joint palette for current pose
    std::vector<glm::mat4>              m_jointPalette;
    // World-space joint positions (for skeleton debug draw, separate from skinning palette)
    std::vector<glm::vec3>              m_jointWorldPos;

    // Owned GL textures (cleaned up in Clear())
    std::vector<GLuint>                 m_ownedTextures;

    Domain::BoundingBox                         m_bounds;

    bool                                m_diagDone = false; // reset on Clear()
    bool                                m_debugDisableSkin = false; // debug toggle

    // Animation
    std::shared_ptr<Parsers::AnimationData>      m_animData;
    std::unique_ptr<AnimationPlayer>    m_animPlayer;
    // Tracks the player time we've last reflected into m_jointPalette so we
    // can detect external SetTime/SetFrame calls while paused and rebuild.
    float                               m_lastAppliedAnimTime = -1.0f;

    // Screen-space outline (Blender-style)
    GLuint m_maskFbo = 0;
    GLuint m_maskTex = 0;       // R8 single-channel mask
    GLuint m_maskDepth = 0;     // depth renderbuffer for mask pass
    int    m_maskW = 0, m_maskH = 0;

    void EnsureMaskFBO(int w, int h);
    void RenderOutlineScreenSpace(const glm::mat4& view, const glm::mat4& proj,
                                  int viewportW, int viewportH);
};

} // namespace Onyx
