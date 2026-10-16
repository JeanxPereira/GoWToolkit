#include <doctest/doctest.h>

#include "core/parsers/gowr/TextureDecode.h"

using Onyx::GOWRTextureStorage;

TEST_CASE("GOWR AGC texture formats retain their source storage") {
    CHECK(Onyx::GOWRTextureStorageFromDataFormat(0x29) == GOWRTextureStorage::BC1);
    CHECK(Onyx::GOWRTextureStorageFromDataFormat(0x2A) == GOWRTextureStorage::BC1);
    CHECK(Onyx::GOWRTextureStorageFromDataFormat(0x2F) == GOWRTextureStorage::BC4);
    CHECK(Onyx::GOWRTextureStorageFromDataFormat(0x31) == GOWRTextureStorage::BC5);
    CHECK(Onyx::GOWRTextureStorageFromDataFormat(0x35) == GOWRTextureStorage::BC7);
    CHECK(Onyx::GOWRTextureStorageFromDataFormat(0x36) == GOWRTextureStorage::BC7);
    CHECK(Onyx::GOWRTextureStorageFromDataFormat(0x38) == GOWRTextureStorage::RGBA8);
    CHECK(Onyx::GOWRTextureStorageFromDataFormat(0x00) == GOWRTextureStorage::Unknown);
}

TEST_CASE("GOWR normal metadata adapts source storage to the generic Onyx contract") {
    const auto bc5 = Onyx::GOWRNormalMapDescFor(GOWRTextureStorage::BC5);
    CHECK(bc5.encoding == Onyx::Parsers::NormalMapEncoding::XYReconstructZ);
    CHECK(bc5.decodeScale == doctest::Approx(2.0f));
    CHECK(bc5.decodeBias == doctest::Approx(-1.0f));

    const auto bc7 = Onyx::GOWRNormalMapDescFor(GOWRTextureStorage::BC7);
    CHECK(bc7.encoding == Onyx::Parsers::NormalMapEncoding::XYReconstructZ);

    const auto athenaConvention = Onyx::GOWRNormalConventionFromShaderName(
        "14ef18dd481d00dc_ps_10000207");
    CHECK(athenaConvention == Onyx::GOWRNormalConvention::AuthoredXYZEndpoint);
    const auto athena = Onyx::GOWRNormalMapDescFor(GOWRTextureStorage::BC7,
                                                   athenaConvention);
    CHECK(athena.encoding == Onyx::Parsers::NormalMapEncoding::XYZ);
    CHECK(athena.decodeScale == doctest::Approx(2.007874011993408f));
    CHECK(athena.decodeBias == doctest::Approx(-1.0078740119934082f));

    const auto unknownShader = Onyx::GOWRNormalConventionFromShaderName(
        "another_material_ps_10000207");
    CHECK(unknownShader == Onyx::GOWRNormalConvention::ConservativeXY);

    const auto bc5Athena = Onyx::GOWRNormalMapDescFor(GOWRTextureStorage::BC5,
                                                      athenaConvention);
    CHECK(bc5Athena.encoding == Onyx::Parsers::NormalMapEncoding::XYReconstructZ);

    const auto rgba = Onyx::GOWRNormalMapDescFor(GOWRTextureStorage::RGBA8,
                                                 athenaConvention);
    CHECK(rgba.encoding == Onyx::Parsers::NormalMapEncoding::XYZ);

    const auto unknown = Onyx::GOWRNormalMapDescFor(GOWRTextureStorage::Unknown);
    CHECK(unknown.encoding == Onyx::Parsers::NormalMapEncoding::XYReconstructZ);
    CHECK(unknown.decodeScale == doctest::Approx(2.0f));
    CHECK(unknown.decodeBias == doctest::Approx(-1.0f));
}
