#include <doctest/doctest.h>
#include <Onyx/Services/AssetVisibility.h>
#include "core/types/GameTypes.h"
#include <Onyx/Types/TypeCatalog.h>

using namespace Onyx::Services;
using namespace Onyx::Domain;
using namespace Onyx::Types;
// Make GameTypes accessible (it lives in Onyx::GameTypes, a sub-ns of Onyx)
using namespace Onyx;

TEST_CASE("Visibility override survives Export/Import by stable value") {
    GameTypes::RegisterGameTypes();

    // A registered handle's value must equal its legacy enum position so the
    // SerializedOverride.typeId stays stable across releases.
    // Mesh is at enum position 9, Texture at 11 in the original TypeId order.
    CHECK(GameTypes::Mesh.value    == 9);
    CHECK(GameTypes::Texture.value == 11);

    auto& vis = AssetVisibility::Get();
    vis.ResetAllOverrides();

    // GOW2 Mesh defaults to Visible; forcing it Hidden creates a real override.
    vis.SetUserOverride(GameTypes::Mesh, /*visible=*/false);
    REQUIRE(vis.GetCurrent(GameTypes::Mesh) == Visibility::Hidden);

    auto blob = vis.ExportOverrides();
    REQUIRE(blob.size() == 1);

    // Round-trip: clear in-memory state, then reload from the serialized blob.
    vis.ResetAllOverrides();
    REQUIRE(vis.GetCurrent(GameTypes::Mesh) == Visibility::Visible);

    vis.ImportOverrides(blob);

    CHECK(vis.GetCurrent(GameTypes::Mesh) == Visibility::Hidden);
    CHECK(vis.IsVisible(GameTypes::Mesh) == false);

    // Leave global singleton state clean for other test cases.
    vis.ResetAllOverrides();
}
