#pragma once

#include <cstdint>

namespace Onyx::Domain {

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

/// Returns a human-readable name for a MediaKind.
const char* Name(MediaKind kind);

/// Returns an SF Symbol icon string for a MediaKind.
const char* Icon(MediaKind kind);

} // namespace Onyx::Domain
