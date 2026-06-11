#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include "MeshData.h"
#include "ObjectData.h"
#include "TextureData.h"
#include "AnimationData.h"

namespace Onyx::Parsers {


/// Blend mode for a render batch, parsed from mat.go flags
enum class BlendMode : uint8_t {
    Normal,       // Standard alpha: SRC_ALPHA, ONE_MINUS_SRC_ALPHA
    Additive,     // Glow/fire:      SRC_ALPHA, ONE
    Subtractive,  // Shadows
    EnvMap        // Environment map blend (strange blended in Go)
};

/// Material info resolved from the WAD, ready to render.
/// Does NOT own the GL texture — that's created by the renderer.
struct MaterialInfo {
    float       baseColor[4] = {1, 1, 1, 1};      // from mat header

    struct Layer {
        std::string textureName;                    // resolved TXR name
        float       blendColor[4] = {1, 1, 1, 1};  // per-layer tint
        BlendMode   blendMode = BlendMode::Normal;
        bool        hasTexture = false;
        float       uvOffset[2] = {0, 0};           // UV scroll offset
    };
    std::vector<Layer> layers;
};

/// Complete render-ready scene built from Instance → Object → Model chain.
/// One of these is produced per viewport/viewer. Owns all the data.
struct SceneData {
    // Transform from Instance (position/rotation/scale in world)
    glm::mat4                                   instanceTransform {1.0f};

    // If true, apply scale(1,1,-1) on top of instanceTransform.
    // GOW2 models face -Z and need this; GOWR is already in screen-correct space.
    bool                                        flipZ = true;

    // If true, this scene is a sky dome (rendered with rotation-only view, depth cleared after)
    bool                                        isSky = false;

    // Skeleton from Object (empty for static meshes)
    std::shared_ptr<ObjectData>                 skeleton;

    // Animation clips from ANM child (shared to allow player to reference)
    std::shared_ptr<AnimationData>              animations;

    // Geometry from Mesh (multiple parts, each with materialId + jointMap)
    std::vector<MeshPart>                       meshParts;

    // Materials from Model's child Material nodes (index = materialId)
    std::vector<MaterialInfo>                   materials;

    // Decoded textures ready for GPU upload (index = materialId, then layerId)
    std::vector<std::vector<std::unique_ptr<TextureData>>>   textures;

    bool HasSkeleton() const {
        return skeleton && skeleton->HasSkeleton();
    }

    bool IsEmpty() const {
        return meshParts.empty();
    }
};

} // namespace Onyx::Parsers
