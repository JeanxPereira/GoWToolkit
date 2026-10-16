#include <doctest/doctest.h>

#include "cli/TextureOutputPath.h"

TEST_CASE("Texture output accepts only the fixed export filename") {
    CHECK(Onyx::Cli::IsSafeTextureOutputFileName("texture.png"));
    CHECK_FALSE(Onyx::Cli::IsSafeTextureOutputFileName("athena.png"));
    CHECK_FALSE(Onyx::Cli::IsSafeTextureOutputFileName("TEXTURE.PNG"));
}

TEST_CASE("Texture output rejects paths and non-PNG destinations") {
    CHECK_FALSE(Onyx::Cli::IsSafeTextureOutputFileName(""));
    CHECK_FALSE(Onyx::Cli::IsSafeTextureOutputFileName("../escape.png"));
    CHECK_FALSE(Onyx::Cli::IsSafeTextureOutputFileName("sub/escape.png"));
    CHECK_FALSE(Onyx::Cli::IsSafeTextureOutputFileName("sub\\escape.png"));
    CHECK_FALSE(Onyx::Cli::IsSafeTextureOutputFileName("C:\\escape.png"));
    CHECK_FALSE(Onyx::Cli::IsSafeTextureOutputFileName("athena.jpg"));
}
