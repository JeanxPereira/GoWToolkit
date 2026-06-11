#include <doctest/doctest.h>
#include "Ui/ViewerRegistry.h"
#include "Core/Domain/MediaKind.h"
#include "core/WadTypes.h"
#include "Core/Types/TypeId.h"
#include "core/types/GameTypes.h"
#include "Ui/Viewers/IDocumentContent.h"

using namespace Onyx;
using namespace Onyx::Domain;
using namespace Onyx::Types;

TEST_CASE("ViewerRegistry Open logic") {
    GameTypes::RegisterGameTypes();
    ViewerRegistry registry;
    AssetContainer dummyWad;

    SUBCASE("Unknown kind returns nullptr") {
        AssetEntry entry;
        entry.kind = MediaKind::Unknown;
        entry.typeId = GameTypes::Unknown;
        
        auto viewer = registry.Open(entry, dummyWad);
        CHECK(viewer == nullptr);
    }

    SUBCASE("Image kind with Unknown TypeId returns nullptr") {
        AssetEntry entry;
        entry.kind = MediaKind::Image;
        entry.typeId = GameTypes::Unknown;
        
        auto viewer = registry.Open(entry, dummyWad);
        CHECK(viewer == nullptr);
    }

    // We can't easily test a valid ImageViewer instantiation here without 
    // an OpenGL context (which ImGui needs) because ImageViewer constructor 
    // usually does OpenGL calls. We just check the interface contract:
    // It should delegate and since GameTypes::Texture might need a valid TextureData, 
    // it will return nullptr or crash if we pass dummy data. 
    // But the AC asks: Para asset com kind == Unknown, retorna nullptr
}
