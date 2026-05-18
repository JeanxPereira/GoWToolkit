#include <doctest/doctest.h>

#include "golden_helpers.h"

namespace gtest = gowtoolkit::testing;

TEST_CASE("[Golden] GOW2 wad_minimal snapshot stability") {
    gtest::RunGoldenTest(
        "gow2",
        gtest::FixturePath("gow2/wad_minimal.wad"),
        gtest::FixturePath("gow2/wad_minimal.expected.json"));
}
