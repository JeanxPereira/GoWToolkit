// Guards the generated smschema tables.
//
// The tables are machine-written from GoWR.exe and are regenerated whenever
// the game patches, so nothing here checks a hand-maintained value. What it
// checks is that a regeneration produced something SHAPED like the schema:
// the invariants the extraction depends on, plus a handful of structs whose
// contents were verified against the binary by hand and would reveal a silent
// change of meaning in the record layout.
//
// The anchors matter more than they look. An earlier revision of this table
// grouped fields by ownerStructId alone, which is library-local, and merged
// every library's struct 170 into one 3000-field struct whose field names
// repeated. Every count in that table was wrong and every lookup returned
// fields belonging to some other type -- and nothing failed, because there was
// no test. AnchoredStructs and LibraryLocalIds below are what would have
// caught it.

#include <doctest/doctest.h>

#include "core/formats/gowr/SmSchema.h"

#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace Onyx::Gowr;

TEST_CASE("SmSchema table invariants hold" * doctest::test_suite("[SmSchema]")) {
    std::vector<std::string> problems;
    const bool ok = SmSchema::Validate(problems);
    for (const auto& p : problems) MESSAGE(p);
    CHECK(problems.empty());
    CHECK(ok);
}

TEST_CASE("SmSchema table is populated" * doctest::test_suite("[SmSchema]")) {
    const SmSchema::Stats st = SmSchema::GetStats();

    // Lower bounds, not exact counts: a game patch may legitimately add or
    // drop types, and pinning the numbers would turn every patch into a test
    // failure. An empty or half-extracted table is what this catches.
    CHECK(st.structs > 2000);
    CHECK(st.fields > 15000);
    CHECK(st.libraries > 500);
    CHECK(st.distinctFieldNames > 5000);

    // Names come from a separate join that can legitimately cover only part of
    // the table -- the descriptors in the image are mostly for types this
    // table does not describe. Assert only that the join produced something.
    CHECK(st.namedStructs > 100);
    CHECK(st.namedStructs < st.structs);
}

TEST_CASE("SmSchema anchored structs match the binary" * doctest::test_suite("[SmSchema]")) {
    // Each of these was read out of GoWR.exe by hand and cross-checked: the
    // field count and runtime size come from the type descriptor, the field
    // names and offsets from the field table, and the two agree.
    SUBCASE("dctools.AutomaticWetnessFromHeight") {
        const auto* s = SmSchema::FindStruct(26, 0x006A);
        REQUIRE(s != nullptr);
        REQUIRE(s->name != nullptr);
        CHECK(std::string(s->name) == "dctools.AutomaticWetnessFromHeight");
        CHECK(s->fieldCount == 8);
        CHECK(s->runtimeSize == 48);
        CHECK(std::string(s->fields[0].name) == "WetnessTintColor");

        const auto* f = SmSchema::FindField(26, 0x006A, "WetnessAmount");
        REQUIRE(f != nullptr);
        CHECK(f->offset == 24);
        CHECK(f->size == 4);
        CHECK(f->type == static_cast<uint16_t>(SmSchema::SmType::Float));
    }

    SUBCASE("dctools.Fog") {
        const auto* s = SmSchema::FindStruct(17, 0x0037);
        REQUIRE(s != nullptr);
        REQUIRE(s->name != nullptr);
        CHECK(std::string(s->name) == "dctools.Fog");
        CHECK(s->runtimeSize == 176);

        // The serialised layout carries bookkeeping the runtime struct has no
        // member for: a TemplateSymbol and an "<X>_IsNull" per optional field.
        // That difference is why the descriptor join runs on names and not on
        // byte offsets, so it is worth pinning that it is still present.
        CHECK(SmSchema::FindField(17, 0x0037, "TemplateSymbol") != nullptr);
        CHECK(SmSchema::FindField(17, 0x0037, "LightColor_IsNull") != nullptr);
        CHECK(s->fieldCount > 15);
    }

    SUBCASE("dctools.MaterialDriver") {
        const auto* s = SmSchema::FindStruct(2, 0x0011);
        REQUIRE(s != nullptr);
        REQUIRE(s->name != nullptr);
        CHECK(std::string(s->name) == "dctools.MaterialDriver");
        CHECK(s->fieldCount == 58);
        CHECK(s->runtimeSize == 1456);
    }

    SUBCASE("the material constant struct") {
        // A material override is a hashed name and a float -- which is exactly
        // why a GOWR MAT entry stores a nameHash and never the text. This is
        // the schema's own statement of that, and the material work leans on
        // it, so it is pinned here.
        const auto* s = SmSchema::FindStruct(SmSchema::kMaterialConstantLibrary,
                                             SmSchema::kMaterialConstantStructId);
        REQUIRE(s != nullptr);
        CHECK(s->fieldCount == 2);

        const auto* name = SmSchema::FindField(SmSchema::kMaterialConstantLibrary,
                                               SmSchema::kMaterialConstantStructId,
                                               "MaterialConstantName");
        REQUIRE(name != nullptr);
        CHECK(name->type == static_cast<uint16_t>(SmSchema::SmType::StringHash));
        CHECK(name->size == 8);

        const auto* value = SmSchema::FindField(SmSchema::kMaterialConstantLibrary,
                                                SmSchema::kMaterialConstantStructId,
                                                "Value");
        REQUIRE(value != nullptr);
        CHECK(value->type == static_cast<uint16_t>(SmSchema::SmType::Float));
        CHECK(value->size == 4);
    }
}

TEST_CASE("SmSchema struct ids are library-local" * doctest::test_suite("[SmSchema]")) {
    // The defect this guards: an id alone does not identify a struct. Find one
    // id that several libraries use and prove they describe different types.
    int idsSharedAcrossLibraries = 0;
    int provenDifferent = 0;

    std::set<uint16_t> ids;
    for (const auto& s : SmSchema::kStructs) ids.insert(s.id);

    for (uint16_t id : ids) {
        std::vector<const SmSchema::Struct*> same;
        for (const auto& s : SmSchema::kStructs)
            if (s.id == id) same.push_back(&s);
        if (same.size() < 2) continue;
        ++idsSharedAcrossLibraries;

        for (size_t i = 1; i < same.size(); ++i) {
            const bool differentShape = same[i]->fieldCount != same[0]->fieldCount ||
                                        std::strcmp(same[i]->fields[0].name,
                                                    same[0]->fields[0].name) != 0;
            if (differentShape) { ++provenDifferent; break; }
        }
    }

    // Measured on the current extraction: 106 ids are used by more than one
    // library, and 20 of those describe genuinely different types. The other
    // 86 are the same type re-registered -- dctools.AnimNode and friends --
    // which is legitimate and is why this is not "all of them". Thresholds sit
    // below the measurements so a patch that shifts the counts does not fail
    // the suite, while an extraction that lost the library dimension (every id
    // unique, or every shared id identical) still does.
    CHECK(idsSharedAcrossLibraries > 50);
    CHECK(provenDifferent > 10);

    // And the pair really does disambiguate: no (library, id) is issued twice.
    // Validate() checks this too, but stating it here is what makes this test
    // readable as the description of the defect.
    std::set<std::pair<uint16_t, uint16_t>> pairs;
    for (const auto& s : SmSchema::kStructs) pairs.insert({s.library, s.id});
    CHECK(static_cast<int>(pairs.size()) == SmSchema::kStructCount);
}

TEST_CASE("SmSchema lookups answer and refuse correctly"
          * doctest::test_suite("[SmSchema]")) {
    SUBCASE("a missing struct is nullptr, not a neighbour") {
        CHECK(SmSchema::FindStruct(60000, 0x0001) == nullptr);
        CHECK(SmSchema::FindField(60000, 0x0001, "Anything") == nullptr);
        CHECK(SmSchema::FindStructByName("nope.NoSuchType") == nullptr);
        CHECK(SmSchema::FindStructsNamed("nope.NoSuchType").empty());
        CHECK(SmSchema::FindStructsWithField("NoSuchFieldName").empty());
    }

    SUBCASE("null arguments are refused rather than dereferenced") {
        CHECK(SmSchema::FindStructByName(nullptr) == nullptr);
        CHECK(SmSchema::FindField(26, 0x006A, nullptr) == nullptr);
        CHECK(SmSchema::FindStructsNamed(nullptr).empty());
        CHECK(SmSchema::FindStructsWithField(nullptr).empty());
    }

    SUBCASE("a type name can be registered by several libraries") {
        // Every animation node library re-declares dctools.AnimNode, so the
        // plural lookup must return more than the singular one does.
        const auto all = SmSchema::FindStructsNamed("dctools.AnimNode");
        CHECK(all.size() > 1);
        const auto* first = SmSchema::FindStructByName("dctools.AnimNode");
        REQUIRE(first != nullptr);
        CHECK(first == all.front());
        std::set<uint16_t> libs;
        for (const auto* s : all) libs.insert(s->library);
        CHECK(libs.size() == all.size());   // one per library, never repeated
    }

    SUBCASE("field lookup by offset agrees with lookup by name") {
        const auto* byName = SmSchema::FindField(26, 0x006A, "WetnessAmount");
        REQUIRE(byName != nullptr);
        CHECK(SmSchema::FindFieldAt(26, 0x006A, byName->offset) == byName);
        CHECK(SmSchema::FindFieldAt(26, 0x006A, 60000) == nullptr);
    }

    SUBCASE("FindStructsWithField finds the struct that owns it") {
        const auto hits = SmSchema::FindStructsWithField("WetnessAmount");
        REQUIRE(!hits.empty());
        bool sawAnchor = false;
        for (const auto* s : hits)
            if (s->library == 26 && s->id == 0x006A) sawAnchor = true;
        CHECK(sawAnchor);
    }
}

TEST_CASE("SmSchema type codes always render" * doctest::test_suite("[SmSchema]")) {
    // Twenty-two distinct codes appear in the table and SmType names twenty of
    // them; two show up once each, on a zero-sized field, and are deliberately
    // left unnamed rather than guessed at. Either way a caller printing a
    // field must get a string back.
    for (const auto& s : SmSchema::kStructs) {
        for (uint16_t i = 0; i < s.fieldCount; ++i) {
            const char* n = SmSchema::TypeName(s.fields[i].type);
            REQUIRE(n != nullptr);
            REQUIRE(n[0] != '\0');
        }
    }
    CHECK(std::string(SmSchema::TypeName(
              static_cast<uint16_t>(SmSchema::SmType::StringHash))) == "stringHash");
    CHECK(std::string(SmSchema::TypeName(0xBEEF)) == "unknown(0xBEEF)");
}
