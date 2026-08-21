#include <doctest/doctest.h>

#include <Onyx/Domain/Entry.h>
#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Types/TypeRegistrar.h>
#include "core/profiles/gowr/GowrTaxonomy.h"
#include "core/modules/GowrModule.h"

using namespace Onyx;
using namespace Onyx::Domain;
using namespace Onyx::Gowr;

namespace {

AssetEntry MakeEntry(const std::string& name, uint64_t size) {
    AssetEntry e;
    e.name         = name;
    e.source.size  = size;
    return e;
}

// A node as the builder actually emits it: typed. The typeId is what
// distinguishes a synthesised pair node from the raw entry it was built from.
AssetEntry MakeTypedEntry(const std::string& name, uint64_t size, Onyx::Types::TypeId type) {
    AssetEntry e = MakeEntry(name, size);
    e.typeId     = type;
    return e;
}

// GowrTaxonomy.cpp's Classify() looks up "gowr.texturePair" by catalog key
// (Phase 2 Task 4 convergence -- see GowrTaxonomy.cpp's Classify() comment),
// not the legacy Onyx::GameTypes::TexturePair extern this file used to type
// its pair-node fixture with. Registers GowrModule's real types once (self-
// contained -- does not depend on any other test's ordering) and returns
// the handle a real GowrModule-built tree actually stamps on a synthesised
// texture-pair node, so "a TexturePair-typed node..." below exercises the
// same id production code produces, not a stale/unrelated one.
Types::TypeId TexturePairTypeId() {
    static const Types::TypeId id = [] {
        GowrModule module;
        Types::TypeRegistrar registrar(Types::TypeCatalog::Get(), "gowr");
        module.RegisterTypes(registrar);
        return Types::TypeCatalog::Get().Find("gowr.texturePair");
    }();
    return id;
}

} // namespace

// Derives the GowrProfileTag that WadNodeBuilder used to store on
// AssetEntry::profileTag, purely from the entry's own name + payload size.
// Onyx v1.1 removed both the tag field and all per-entry storage, so this is
// the only way the eight former readers can still get at role/parsedName.

TEST_CASE("GowrClassify: vertex shader HASH_vs_FLAGS") {
    auto tag = Classify(MakeEntry("a1b2c3d4_vs_00000001", 512));
    CHECK(tag.role == WadEntryRole::ShaderVertex);
}

TEST_CASE("GowrClassify: pixel shader HASH_ps_FLAGS") {
    auto tag = Classify(MakeEntry("a1b2c3d4_ps_00000001", 512));
    CHECK(tag.role == WadEntryRole::ShaderPixel);
}

TEST_CASE("GowrClassify: MG_*_gpu mesh") {
    auto tag = Classify(MakeEntry("MG_athena10_0_gpu", 4096));
    CHECK(tag.role == WadEntryRole::MeshGpu);
    CHECK(tag.parsedName.prefix == "MG");
}

TEST_CASE("GowrClassify: ANM_* clip") {
    auto tag = Classify(MakeEntry("ANM_athena10_run", 2048));
    CHECK(tag.role == WadEntryRole::AnimClip);
}

TEST_CASE("GowrClassify: MAT_HASH with size > 0 is Material") {
    auto tag = Classify(MakeEntry("MAT_DE674F96622453EB", 128));
    CHECK(tag.role == WadEntryRole::Material);
}

TEST_CASE("GowrClassify: MAT_HASH with size == 0 is MaterialRef") {
    auto tag = Classify(MakeEntry("MAT_DE674F96622453EB", 0));
    CHECK(tag.role == WadEntryRole::MaterialRef);
}

// The GPU/CPU texture split uses the same threshold WadNodeBuilder's
// ClassifyByName has always used: size >= 1024 is the "large" GPU payload,
// below that is the "small" CPU descriptor (WadNodeBuilder.cpp:180 prior to
// this change; now GowrTaxonomy.cpp).
TEST_CASE("GowrClassify: an UNTYPED TX_* at/above 1024 bytes classifies as TextureGpu") {
    auto atThreshold  = Classify(MakeEntry("TX_angrboda_head_0d_1D293ECA4DE04637", 1024));
    auto aboveThreshold = Classify(MakeEntry("TX_angrboda_head_0d_1D293ECA4DE04637", 65536));
    CHECK(atThreshold.role == WadEntryRole::TextureGpu);
    CHECK(aboveThreshold.role == WadEntryRole::TextureGpu);
}

TEST_CASE("GowrClassify: an UNTYPED TX_* below 1024 bytes classifies as TextureCpu") {
    auto tag = Classify(MakeEntry("TX_angrboda_head_0d_1D293ECA4DE04637", 1023));
    CHECK(tag.role == WadEntryRole::TextureCpu);
}

// The case the raw size split gets WRONG for real tree nodes, and the reason
// Classify consults typeId for textures.
//
// No raw texture entry ever becomes a node: the builder consumes each
// TextureCpu into its matching TextureGpu and emits ONE node for the pair,
// which the pre-port code relabelled TexturePair by overwriting the stored tag.
// That node keeps the GPU entry's large size, so size-based classification
// would report TextureGpu -- exactly the label the builder overwrote. Losing
// this would mislabel every texture in the browser and the inspector.
TEST_CASE("GowrClassify: a TexturePair-typed node stays TexturePair despite GPU size") {
    auto pairNode = MakeTypedEntry("TX_angrboda_head_0d_1D293ECA4DE04637", 65536,
                                    TexturePairTypeId());
    CHECK(Classify(pairNode).role == WadEntryRole::TexturePair);

    // And a small one too -- the pair node's size is whatever the GPU side had,
    // so neither side of the threshold may flip it.
    auto smallPair = MakeTypedEntry("TX_angrboda_head_0d_1D293ECA4DE04637", 512,
                                     TexturePairTypeId());
    CHECK(Classify(smallPair).role == WadEntryRole::TexturePair);
}

TEST_CASE("GowrClassify: unrecognised name falls to Unknown") {
    auto tag = Classify(MakeEntry("not_a_known_pattern_at_all", 64));
    CHECK(tag.role == WadEntryRole::Unknown);
}

TEST_CASE("GowrClassify: parsedName is derived, not stored") {
    auto tag = Classify(MakeEntry("MG_heroa00_0_gpu---00042.bin", 4096));
    CHECK(tag.parsedName.prefix == "MG");
    CHECK(tag.parsedName.base == "heroa00");
    CHECK(tag.parsedName.lod == 0);
    CHECK(tag.parsedName.variant == "gpu");
    CHECK(tag.parsedName.wadIndex == 42);
}
