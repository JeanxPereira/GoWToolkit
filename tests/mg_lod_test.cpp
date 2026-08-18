#include <doctest/doctest.h>
#include "core/parsers/gowr/MgParser.h"
#include <Onyx/Vfs/MemoryFile.h>
#include <cstring>
#include <initializer_list>
#include <vector>

// Detail levels in GOWR are declared by the MG part table, not inferred from
// submesh order or from repeated LOD keys. Two things about that layout are
// easy to get wrong and fail silently:
//
//   * a level block's Kind is a submesh *count*, so Kind == 2 means the level
//     is split across two submeshes drawn together. Reading only the first one
//     drops half the geometry of every high level.
//   * the trailing block at distance 32767 is a real level when Kind >= 1 and
//     a "draw nothing" marker when Kind == 0. Treating it as always-terminator
//     loses a part's coarsest level.
//
// Shape below mirrors r_athena00: part 0 ends culled, part 1 keeps its lowest
// level forever and has a Kind == 2 level.

namespace {

struct MgBuilder {
    std::vector<uint8_t> buf;

    void need(size_t end) { if (buf.size() < end) buf.resize(end, 0); }
    template <typename T> void put(size_t off, T v) {
        need(off + sizeof(T));
        std::memcpy(buf.data() + off, &v, sizeof(T));
    }
    // Kind is the submesh count and the indices follow it, so a block grows
    // with the level. Counts above two are common in shipped content.
    void level(size_t at, uint32_t kind, float dist,
               std::initializer_list<uint16_t> sms) {
        put<uint32_t>(at + 0x00, kind);
        put<float>   (at + 0x04, dist);
        put<uint16_t>(at + 0x08, 0);
        size_t o = at + 0x0A;
        for (uint16_t s : sms) { put<uint16_t>(o, s); o += 2; }
    }
};

// part 0: sm0 -> sm1, then culled.      palette {7, 9}
// part 1: sm2+sm3 -> sm4, kept forever. palette {3}
std::vector<uint8_t> BuildMg() {
    MgBuilder b;
    const uint16_t partCount = 2;
    b.put<uint16_t>(0x30, partCount);

    const size_t ptrTable   = 0x44;
    const size_t rangeTable = ptrTable + partCount * 4;
    const size_t palette    = rangeTable + partCount * 4;
    const size_t part0      = palette + 3 * 20;   // 3 palette entries, 20 bytes each
    const size_t part1      = part0 + 0x80;

    b.put<uint32_t>(ptrTable + 0, (uint32_t)part0);
    b.put<uint32_t>(ptrTable + 4, (uint32_t)part1);

    // Cumulative (start, count) ranges.
    b.put<uint16_t>(rangeTable + 0, 0);  b.put<uint16_t>(rangeTable + 2, 2);
    b.put<uint16_t>(rangeTable + 4, 2);  b.put<uint16_t>(rangeTable + 6, 1);

    b.put<uint16_t>(palette + 0 * 20, (uint16_t)7);
    b.put<uint16_t>(palette + 1 * 20, (uint16_t)9);
    b.put<uint16_t>(palette + 2 * 20, (uint16_t)3);

    // part 0 - three blocks, the last one culls.
    b.put<uint16_t>(part0 + 0x00, (uint16_t)11);   // boneRef
    b.put<uint8_t> (part0 + 0x02, (uint8_t)3);     // level count incl. terminator
    b.put<uint8_t> (part0 + 0x03, (uint8_t)2);     // skinned
    b.put<uint32_t>(part0 + 0x38, 0x50);
    b.put<uint32_t>(part0 + 0x3C, 0x60);
    b.put<uint32_t>(part0 + 0x40, 0x70);
    b.level(part0 + 0x50, 1, 6.0f, { 0 });
    b.level(part0 + 0x60, 1, 30.0f, { 1 });
    b.level(part0 + 0x70, 0, 32767.0f, { 0 });

    // part 1 - a two-submesh level, then a level kept at any range.
    b.put<uint16_t>(part1 + 0x00, (uint16_t)4);
    b.put<uint8_t> (part1 + 0x02, (uint8_t)2);
    b.put<uint8_t> (part1 + 0x03, (uint8_t)0);     // rigid
    b.put<uint32_t>(part1 + 0x38, 0x50);
    b.put<uint32_t>(part1 + 0x3C, 0x60);
    b.level(part1 + 0x50, 2, 5.0f, { 2, 3 });
    b.level(part1 + 0x60, 1, 32767.0f, { 4 });

    b.need(part1 + 0x70);
    return b.buf;
}

Onyx::GOWRMgParser::Data ParseFixture() {
    auto bytes = BuildMg();
    auto file  = std::make_shared<Onyx::Vfs::MemoryFile>(std::move(bytes));
    Onyx::GOWRMgParser::Data d;
    REQUIRE(Onyx::GOWRMgParser::Parse(file, 5, d));
    return d;
}

} // namespace

TEST_CASE("MG part table yields one chain per part") {
    auto d = ParseFixture();
    REQUIRE(d.parts.size() == 2);
    CHECK(d.parts[0].levels.size() == 2);   // terminator dropped
    CHECK(d.parts[1].levels.size() == 2);   // trailing level kept
    CHECK(d.MaxLevelCount() == 2);
}

TEST_CASE("A Kind==2 level carries both of its submeshes") {
    auto d = ParseFixture();
    const auto& lv = d.parts[1].levels[0];
    REQUIRE(lv.submeshes.size() == 2);
    CHECK(lv.submeshes[0] == 2);
    CHECK(lv.submeshes[1] == 3);
    CHECK(lv.maxDistance == doctest::Approx(5.0f));
}

TEST_CASE("A level can name more than two submeshes") {
    // r_heroa00 has 36 level blocks with counts of 3, 4, 6 and 10 across 23
    // parts. Capping at two drops them and the part loses whole detail levels.
    MgBuilder b;
    b.put<uint16_t>(0x30, (uint16_t)1);
    const size_t ptrTable = 0x44, rangeTable = 0x48, palette = 0x4C, part = 0x100;
    b.put<uint32_t>(ptrTable, (uint32_t)part);
    b.put<uint16_t>(rangeTable + 0, 0);
    b.put<uint16_t>(rangeTable + 2, 1);
    b.put<uint16_t>(palette, (uint16_t)5);

    b.put<uint16_t>(part + 0x00, (uint16_t)0);
    b.put<uint8_t> (part + 0x02, (uint8_t)2);
    b.put<uint8_t> (part + 0x03, (uint8_t)2);
    b.put<uint32_t>(part + 0x38, 0x50);
    b.put<uint32_t>(part + 0x3C, 0x68);   // 24 bytes on, as a four-submesh block needs
    b.level(part + 0x50, 4, 4.4f, { 10, 11, 12, 13 });
    b.level(part + 0x68, 3, 32767.0f, { 14, 15, 16 });
    b.need(part + 0x80);

    auto file = std::make_shared<Onyx::Vfs::MemoryFile>(std::move(b.buf));
    Onyx::GOWRMgParser::Data d;
    REQUIRE(Onyx::GOWRMgParser::Parse(file, 20, d));
    REQUIRE(d.parts.size() == 1);
    REQUIRE(d.parts[0].levels.size() == 2);
    CHECK(d.parts[0].levels[0].submeshes == std::vector<uint16_t>{ 10, 11, 12, 13 });
    CHECK(d.parts[0].levels[1].submeshes == std::vector<uint16_t>{ 14, 15, 16 });
    CHECK(d.partOfSubmesh[13] == 0);
    CHECK(d.levelOfSubmesh[16] == 1);
}

TEST_CASE("A trailing block only terminates when it draws nothing") {
    auto d = ParseFixture();
    CHECK(d.parts[0].culledAtRange);            // Kind == 0 at 32767
    CHECK_FALSE(d.parts[1].culledAtRange);      // Kind == 1 at 32767 is a level
    CHECK(d.parts[1].levels[1].submeshes.size() == 1);
    CHECK(d.parts[1].levels[1].submeshes[0] == 4);
}

TEST_CASE("Every submesh maps back to its part and level") {
    auto d = ParseFixture();
    const int expectPart[5]  = { 0, 0, 1, 1, 1 };
    const int expectLevel[5] = { 0, 1, 0, 0, 1 };
    for (int sm = 0; sm < 5; ++sm) {
        CHECK(d.partOfSubmesh[sm]  == expectPart[sm]);
        CHECK(d.levelOfSubmesh[sm] == expectLevel[sm]);
    }
}

TEST_CASE("Bone palette is sliced per part from the cumulative range table") {
    auto d = ParseFixture();
    REQUIRE(d.parts[0].palette.size() == 2);
    CHECK(d.parts[0].palette[0] == 7);
    CHECK(d.parts[0].palette[1] == 9);
    REQUIRE(d.parts[1].palette.size() == 1);
    CHECK(d.parts[1].palette[0] == 3);
    CHECK(d.parts[1].rigid);
}

TEST_CASE("A non-cumulative range table is rejected rather than misread") {
    auto bytes = BuildMg();
    // Corrupt the second range's start so it no longer follows the first.
    const size_t rangeTable = 0x44 + 2 * 4;
    uint16_t bad = 99;
    std::memcpy(bytes.data() + rangeTable + 4, &bad, sizeof(bad));

    auto file = std::make_shared<Onyx::Vfs::MemoryFile>(std::move(bytes));
    Onyx::GOWRMgParser::Data d;
    REQUIRE(Onyx::GOWRMgParser::Parse(file, 5, d));
    // Levels still resolve; only the palette is dropped.
    CHECK(d.parts[0].levels.size() == 2);
    CHECK(d.parts[0].palette.empty());
    CHECK(d.parts[1].palette.empty());
}
