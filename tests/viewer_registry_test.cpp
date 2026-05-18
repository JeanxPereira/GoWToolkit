#include <doctest/doctest.h>
#include "ui/ViewerRegistry.h"
#include "core/domain/MediaKind.h"
#include "core/WadTypes.h"
#include "core/types/TypeId.h"
#include "ui/viewers/IDocumentContent.h"

using namespace GOW;

TEST_CASE("ViewerRegistry OpenByKind logic") {
    ViewerRegistry registry;
    OpenWad dummyWad;

    SUBCASE("Unknown kind returns nullptr") {
        ParsedEntry entry;
        entry.kind = MediaKind::Unknown;
        entry.typeId = TypeId::Unknown;
        
        auto viewer = registry.OpenByKind(entry, dummyWad);
        CHECK(viewer == nullptr);
    }

    SUBCASE("Image kind with Unknown TypeId returns nullptr") {
        ParsedEntry entry;
        entry.kind = MediaKind::Image;
        entry.typeId = TypeId::Unknown;
        
        auto viewer = registry.OpenByKind(entry, dummyWad);
        CHECK(viewer == nullptr);
    }

    // We can't easily test a valid ImageViewer instantiation here without 
    // an OpenGL context (which ImGui needs) because ImageViewer constructor 
    // usually does OpenGL calls. We just check the interface contract:
    // It should delegate and since TypeId::Texture might need a valid TextureData, 
    // it will return nullptr or crash if we pass dummy data. 
    // But the AC asks: Para asset com kind == Unknown, retorna nullptr
}
