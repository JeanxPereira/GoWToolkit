#pragma once

#include <cstdint>
#include "core/types/TypeId.h"

namespace GOW {

/// Represents the high-level semantic "kind" of media an asset represents,
/// regardless of the underlying game version or specific format.
/// This abstraction allows the UI to be game-agnostic.
enum class MediaKind : uint8_t {
    Unknown,
    Image,    // GOW1/2 TXR, GOWR TexturePair, etc.
    Mesh,     // GOW1/2/R 3D models
    Material, // Material definitions
    Skeleton, // Rig / Bones / Object instances
    Animation,// Animation clips
    Audio,    // VAG, SBK, GOWR audio
    Video,    // VPK, PSS, PSW
    Script,   // Game scripts
    Map,      // Contexts / Scenes
    Shader,   // GPU Shaders
    Container,// Wad, Pak, Iso (navigable archives)
    Raw       // Unknown bytes
};

/// Maps a specific TypeId to its broad MediaKind.
/// This is a constexpr mapping that guarantees every asset type
/// resolves to a semantic kind for routing to the correct viewer.
constexpr MediaKind KindOf(TypeId id) {
    switch (id) {
        // Image
        case TypeId::Texture:
        case TypeId::TexturePair:
            return MediaKind::Image;

        // Mesh
        case TypeId::Mesh:
        case TypeId::Model:
        case TypeId::MeshGpu:
        case TypeId::MeshDefn:
        case TypeId::LodBinding:
            return MediaKind::Mesh;

        // Material
        case TypeId::Material:
        case TypeId::MaterialRef:
            return MediaKind::Material;

        // Skeleton / Hierarchy
        case TypeId::Object:
        case TypeId::GameObjectProto:
        case TypeId::GameObjectInst:
        case TypeId::GameObjectOverride:
            return MediaKind::Skeleton;

        // Animation
        case TypeId::Animation:
        case TypeId::AnimClip:
            return MediaKind::Animation;

        // Audio
        case TypeId::Sound:
        case TypeId::VagAudio:
        case TypeId::SoundEmitter:
            return MediaKind::Audio;

        // Video
        case TypeId::VpkVideo:
        case TypeId::PssVideo:
        case TypeId::PswVideo:
            return MediaKind::Video;

        // Script
        case TypeId::Script:
            return MediaKind::Script;

        // Map / Scene
        case TypeId::Instance:
            return MediaKind::Map;

        // Shader
        case TypeId::ShaderContainer:
        case TypeId::ShaderVertex:
        case TypeId::ShaderPixel:
        case TypeId::ShaderHull:
        case TypeId::ShaderDomain:
        case TypeId::ShaderCompute:
        case TypeId::ShaderLibrary:
            return MediaKind::Shader;

        // Container
        case TypeId::WadFile:
        case TypeId::WadIdentity:
        case TypeId::SharedWadRef:
            return MediaKind::Container;

        // Raw / Data
        case TypeId::GfxData:
        case TypeId::PalData:
        case TypeId::Collision:
        case TypeId::Flipbook:
        case TypeId::Chunk:
        case TypeId::ParticleEmitter:
        case TypeId::ParticleSystem:
        case TypeId::ClientGuid:
        case TypeId::Light:
            return MediaKind::Raw;

        // Structural tags (not media)
        case TypeId::EntityCount:
        case TypeId::GroupStart:
        case TypeId::GroupEnd:
        case TypeId::HeaderStart:
        case TypeId::HeaderPop:
        case TypeId::Sentinel:
        case TypeId::Unknown:
        case TypeId::COUNT:
            return MediaKind::Unknown;
    }
    return MediaKind::Unknown;
}

/// Returns a human-readable name for a MediaKind.
const char* Name(MediaKind kind);

/// Returns an SF Symbol icon string for a MediaKind.
const char* Icon(MediaKind kind);

} // namespace GOW
