#include <doctest/doctest.h>
#include <string>
#include "core/domain/MediaKind.h"
#include "core/types/GameTypes.h"
#include "core/types/TypeCatalog.h"

using namespace Onyx;

TEST_CASE("Catalog media routing") {
    GameTypes::RegisterGameTypes();
    CHECK(KindOf(GameTypes::Mesh)     == MediaKind::Mesh);
    CHECK(KindOf(GameTypes::Texture)  == MediaKind::Image);
    CHECK(KindOf(GameTypes::Sound)    == MediaKind::Audio);
    CHECK(KindOf(GameTypes::Instance) == MediaKind::Map);
}

TEST_CASE("Catalog media coverage") {
    GameTypes::RegisterGameTypes();

    // Explicit AC checks
    CHECK(KindOf(GameTypes::Texture) == MediaKind::Image);
    CHECK(KindOf(GameTypes::VagAudio) == MediaKind::Audio);

    // Some content types
    CHECK(KindOf(GameTypes::Animation) == MediaKind::Animation);
    CHECK(KindOf(GameTypes::Material) == MediaKind::Material);
    CHECK(KindOf(GameTypes::WadFile) == MediaKind::Container);

    // Non-content types
    CHECK(KindOf(GameTypes::GroupStart) == MediaKind::Unknown);
    CHECK(KindOf(GameTypes::Sentinel) == MediaKind::Unknown);

    // Test Name() and Icon()
    CHECK(std::string(Name(MediaKind::Image)) == "Image");
    CHECK(Icon(MediaKind::Image) != nullptr);
}
