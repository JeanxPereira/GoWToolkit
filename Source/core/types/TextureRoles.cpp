#include "core/types/TextureRoles.h"

namespace Onyx {

const char* SceneRoleName(Onyx::Parsers::TextureRole role) {
    switch (role) {
        case Onyx::Parsers::TextureRole::Diffuse:   return "Diffuse";
        case Onyx::Parsers::TextureRole::Normal:    return "Normal";
        case Onyx::Parsers::TextureRole::Occlusion: return "Occlusion";
        case Onyx::Parsers::TextureRole::Gloss:     return "Gloss";
        case Onyx::Parsers::TextureRole::Height:    return "Height";
        case Onyx::Parsers::TextureRole::Scatter:   return "Scatter";
        case Onyx::Parsers::TextureRole::Detail:    return "Detail";
        case Onyx::Parsers::TextureRole::Emissive:  return "Emissive";
        case Onyx::Parsers::TextureRole::EnvMap:    return "EnvMap";
        case Onyx::Parsers::TextureRole::Opacity:   return "Opacity";
    }
    return "?";
}

std::optional<Onyx::Parsers::TextureRole> ToSceneRole(Onyx::TextureRole role) {
    switch (role) {
        case Onyx::TextureRole::Diffuse:          return Onyx::Parsers::TextureRole::Diffuse;
        case Onyx::TextureRole::Normal:           return Onyx::Parsers::TextureRole::Normal;
        case Onyx::TextureRole::AmbientOcclusion: return Onyx::Parsers::TextureRole::Occlusion;
        case Onyx::TextureRole::Height:           return Onyx::Parsers::TextureRole::Height;
        case Onyx::TextureRole::Emissive:         return Onyx::Parsers::TextureRole::Emissive;
        case Onyx::TextureRole::Gloss:            return Onyx::Parsers::TextureRole::Gloss;
        case Onyx::TextureRole::Scatter:          return Onyx::Parsers::TextureRole::Scatter;
        case Onyx::TextureRole::Detail:           return Onyx::Parsers::TextureRole::Detail;
        case Onyx::TextureRole::Opacity:          return Onyx::Parsers::TextureRole::Opacity;

        // Declared by GOWR materials, but the renderer has no sampler bound to
        // them. Decoding one would cost a texture upload nothing reads.
        case Onyx::TextureRole::Specular:
        case Onyx::TextureRole::Roughness:
        case Onyx::TextureRole::Metallic:
        case Onyx::TextureRole::Unknown:
            return std::nullopt;
    }
    return std::nullopt;
}

} // namespace Onyx
