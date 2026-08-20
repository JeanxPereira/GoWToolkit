#include <doctest/doctest.h>

#include <Onyx/Domain/Entry.h>
#include "core/profiles/gowr/GowrTaxonomy.h"

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
TEST_CASE("GowrClassify: TX_* at/above the 1024-byte threshold is TextureGpu") {
    auto atThreshold  = Classify(MakeEntry("TX_angrboda_head_0d_1D293ECA4DE04637", 1024));
    auto aboveThreshold = Classify(MakeEntry("TX_angrboda_head_0d_1D293ECA4DE04637", 65536));
    CHECK(atThreshold.role == WadEntryRole::TextureGpu);
    CHECK(aboveThreshold.role == WadEntryRole::TextureGpu);
}

TEST_CASE("GowrClassify: TX_* below the 1024-byte threshold is TextureCpu") {
    auto tag = Classify(MakeEntry("TX_angrboda_head_0d_1D293ECA4DE04637", 1023));
    CHECK(tag.role == WadEntryRole::TextureCpu);
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
