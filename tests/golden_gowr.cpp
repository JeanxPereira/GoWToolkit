#include <doctest/doctest.h>

#include "golden_helpers.h"

namespace gtest = gowtoolkit::testing;

TEST_CASE("[Golden] GOWR wad_minimal snapshot stability") {
    gtest::RunGoldenTest(
        "gowr",
        gtest::FixturePath("gowr/wad_minimal.wad"),
        gtest::FixturePath("gowr/wad_minimal.expected.json"));
}
