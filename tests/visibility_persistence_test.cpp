// Asset-visibility overrides must survive a save/load round-trip.
//
// The original version of this test asserted persistence by NUMERIC TypeId
// (ExportOverrides/ImportOverrides, plus `CHECK(GameTypes::Mesh.value == 9)`
// to pin the enum position a serialized blob depended on). Onyx v1.1 replaced
// that with SaveOverrides/LoadOverrides keyed on the type's CATALOG KEY, and
// the new shape is what makes the old assertion unnecessary rather than
// merely different: a catalog that registers types in another order, or an app
// that adds one, can no longer corrupt a saved override, so there is nothing
// left for a hardcoded enum position to protect.
//
// What is worth keeping -- an override survives the round-trip, and a key the
// catalog does not know is dropped instead of resurrected -- is asserted
// below.

#include <doctest/doctest.h>
#include <Onyx/Services/AssetVisibility.h>
#include <Onyx/Services/Settings.h>
#include <Onyx/Types/TypeCatalog.h>
#include "core/types/GameTypes.h"

#include <filesystem>
#include <system_error>

using namespace Onyx::Services;
using namespace Onyx::Types;
using namespace Onyx;

namespace {

// Settings is only constructible through Load(), and a missing file yields an
// empty, clean object -- which also lets the round-trip below go through the
// real file rather than an in-memory stand-in.
std::filesystem::path ScratchSettings(const char* stem) {
    return std::filesystem::temp_directory_path() /
           (std::string("gowtoolkit_vis_") + stem + ".json");
}

Settings FreshSettings(const std::filesystem::path& p) {
    std::error_code ec;
    std::filesystem::remove(p, ec);
    return Settings::Load(p);
}

} // namespace

TEST_CASE("Visibility override survives a Save/Load round-trip") {
    GameTypes::RegisterGameTypes();

    auto& catalog = TypeCatalog::Get();
    auto& vis     = AssetVisibility::Get();
    vis.ResetAllOverrides();

    const bool defaultVisible = vis.IsVisible(GameTypes::Mesh);

    // Force the opposite of the default, so the override is a real one rather
    // than a value that happens to match what a cleared state would report.
    vis.SetUserOverride(GameTypes::Mesh, !defaultVisible);
    REQUIRE(vis.IsVisible(GameTypes::Mesh) == !defaultVisible);

    const auto path = ScratchSettings("roundtrip");
    Settings settings = FreshSettings(path);
    vis.SaveOverrides(catalog, settings);

    // Persisted under the type's catalog key, not its numeric value.
    const std::string key = "visibility." + std::string(catalog.KeyOf(GameTypes::Mesh));
    REQUIRE(settings.GetBool(key).has_value());
    CHECK(settings.GetBool(key).value() == !defaultVisible);

    // Clear in-memory state, then reload.
    vis.ResetAllOverrides();
    REQUIRE(vis.IsVisible(GameTypes::Mesh) == defaultVisible);

    // Round-trip through the actual file, not just the in-memory object.
    REQUIRE(settings.Save());
    Settings reloaded = Settings::Load(path);
    REQUIRE(reloaded.GetBool(key).has_value());

    vis.LoadOverrides(catalog, reloaded);
    CHECK(vis.IsVisible(GameTypes::Mesh) == !defaultVisible);

    // Leave the global singleton clean for other test cases.
    vis.ResetAllOverrides();
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("Visibility drops keys the catalog does not know") {
    GameTypes::RegisterGameTypes();

    auto& catalog = TypeCatalog::Get();
    auto& vis     = AssetVisibility::Get();
    vis.ResetAllOverrides();

    // A key from a removed or renamed type. Loading it must not resurrect an
    // override against whatever id happens to occupy that slot now.
    const auto path = ScratchSettings("unknownkey");
    Settings settings = FreshSettings(path);
    settings.Set("visibility.gow2.a-type-that-never-existed", false);

    vis.LoadOverrides(catalog, settings);

    CHECK(vis.ExportOverridesByKey(catalog).empty());

    vis.ResetAllOverrides();
    std::error_code ec;
    std::filesystem::remove(path, ec);
}
