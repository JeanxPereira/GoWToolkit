// Regression: a selection must not outlive the tree it points into.
//
// The original crash: Onyx::Api::GetSelected() handed out a raw AssetEntry*
// into AssetDatabase::wads, closing a WAD erased that vector element, nothing
// cleared the selection, and Inspector::Draw() dereferenced freed memory on
// the very next frame -- the app died right after "Close WAD".
//
// Onyx v1.1 makes that specific crash unrepresentable: AssetDatabase and the
// global selection pointers are gone, and selection is a
// SelectionChanged{DocumentId, NodePath} event carrying pure data. But the
// hazard did not vanish, it changed shape -- a NodePath captured before a
// document closes still names indices into a tree that no longer exists, and
// every holder (Inspector, WadBrowser) re-resolves it each frame. So the
// invariant worth testing is the same one, restated for the model that
// replaced the pointers: after the document goes, resolving must fail
// cleanly rather than reach into anything.

#include <doctest/doctest.h>
#include <Onyx/Modules/Selection.h>
#include <Onyx/Modules/Workspace.h>
#include <Onyx/Types/TypeCatalog.h>

#include "core/modules/Gow2Module.h"
#include "core/modules/GowrModule.h"

#include <filesystem>
#include <memory>

namespace {

std::filesystem::path Gow2Fixture() {
    return std::filesystem::path(GOWTOOLKIT_TEST_FIXTURES_DIR) / "gow2" / "wad_minimal.wad";
}

Onyx::Modules::DocumentId OpenFixture(Onyx::Modules::Workspace& ws) {
    const auto id = ws.Open(Gow2Fixture(), "gow2");
    ws.Events().Pump();
    return id;
}

} // namespace

TEST_CASE("[Selection] a path stops resolving once its document closes") {
    REQUIRE(std::filesystem::exists(Gow2Fixture()));

    Onyx::Modules::Workspace ws(Onyx::Types::TypeCatalog::Get());
    ws.AddModule(std::make_unique<Onyx::Gow2::Gow2Module>());

    const auto id = OpenFixture(ws);
    REQUIRE(id != 0);

    Onyx::Modules::Document* doc = ws.Get(id);
    REQUIRE(doc != nullptr);
    REQUIRE(doc->ready.load());
    REQUIRE_FALSE(doc->roots.empty());

    // Select the first root, the way WadBrowser's click handler would.
    Onyx::Modules::NodePath path;
    path.indices.push_back(0);
    REQUIRE(Onyx::Modules::Resolve(*doc, path) != nullptr);

    ws.Close(id);

    // The document is gone: a holder that kept the path must find nothing
    // rather than dereference anything. Get() returning null is the check a
    // holder makes before it ever reaches Resolve().
    CHECK(ws.Get(id) == nullptr);
}

TEST_CASE("[Selection] a path into a shorter tree fails its bounds check") {
    REQUIRE(std::filesystem::exists(Gow2Fixture()));

    Onyx::Modules::Workspace ws(Onyx::Types::TypeCatalog::Get());
    ws.AddModule(std::make_unique<Onyx::Gow2::Gow2Module>());

    const auto id = OpenFixture(ws);
    REQUIRE(id != 0);
    Onyx::Modules::Document* doc = ws.Get(id);
    REQUIRE(doc != nullptr);

    // A path built against a bigger tree -- the same shape a stale selection
    // takes after a reparse drops nodes.
    Onyx::Modules::NodePath tooDeep;
    tooDeep.indices.push_back(static_cast<uint32_t>(doc->roots.size() + 100));
    CHECK(Onyx::Modules::Resolve(*doc, tooDeep) == nullptr);

    // An empty path never names a node, by contract.
    Onyx::Modules::NodePath empty;
    CHECK(Onyx::Modules::Resolve(*doc, empty) == nullptr);

    ws.Close(id);
}
