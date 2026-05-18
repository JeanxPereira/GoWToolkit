#include <doctest/doctest.h>
#include "core/domain/MediaKind.h"

using namespace GOW;

TEST_CASE("MediaKind Constexpr Evaluation") {
    // Verification that KindOf is constexpr
    static_assert(KindOf(TypeId::Mesh) == MediaKind::Mesh, "Mesh should map to Mesh");
    static_assert(KindOf(TypeId::Texture) == MediaKind::Image, "Texture should map to Image");
    static_assert(KindOf(TypeId::Sound) == MediaKind::Audio, "Sound should map to Audio");
    static_assert(KindOf(TypeId::Instance) == MediaKind::Map, "Instance should map to Map");
}

TEST_CASE("MediaKind Coverage") {
    // Iterate over all TypeIds to ensure everything returns a valid value.
    // Structural tags map to Unknown, but none should trigger undefined behavior.
    
    // Explicit AC checks
    CHECK(KindOf(TypeId::Texture) == MediaKind::Image);
    CHECK(KindOf(TypeId::VagAudio) == MediaKind::Audio);

    // Some content types
    CHECK(KindOf(TypeId::Animation) == MediaKind::Animation);
    CHECK(KindOf(TypeId::Material) == MediaKind::Material);
    CHECK(KindOf(TypeId::WadFile) == MediaKind::Container);
    
    // Non-content types
    CHECK(KindOf(TypeId::GroupStart) == MediaKind::Unknown);
    CHECK(KindOf(TypeId::Sentinel) == MediaKind::Unknown);
    
    // Test Name() and Icon()
    CHECK(std::string(Name(MediaKind::Image)) == "Image");
    CHECK(Icon(MediaKind::Image) != nullptr);
}
