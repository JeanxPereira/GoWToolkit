#include <doctest/doctest.h>
#include "core/parsers/gowr/MaterialParser.h"
#include <Onyx/Vfs/MemoryFile.h>
#include <cstring>
#include <vector>

// A GOWR material spans two WAD entries: the MAT holds the parameter table and
// a companion record lists every asset it pulls in. Two things about that
// companion are easy to get wrong.
//
//   * A reference is identified by the referenced asset's 16-byte WAD GUID.
//     Texture GUIDs begin with a fixed 8-byte tag, which is what
//     separates a texture from a shader without consulting the WAD - matching
//     on the name instead would misfile anything unusually named.
//   * The record's final name field is truncated when the entry runs to the end
//     of the payload, so a reader that insists on a full 76-byte stride drops
//     the last reference.

namespace {

struct RefBuilder {
    std::vector<uint8_t> buf;

    void u32(uint32_t v) {
        const size_t o = buf.size();
        buf.resize(o + 4);
        std::memcpy(buf.data() + o, &v, 4);
    }
    void entry(const uint8_t guid[16], const char* name, bool truncate = false) {
        const size_t o = buf.size();
        buf.resize(o + 16);
        std::memcpy(buf.data() + o, guid, 16);
        // A real file loses only trailing padding, so the short field still
        // holds the whole name plus its terminator.
        const size_t nameBytes = truncate ? 32 : 60;
        const size_t n = buf.size();
        buf.resize(n + nameBytes, 0);
        std::memcpy(buf.data() + n, name, std::min(std::strlen(name), nameBytes - 1));
    }
};

std::vector<uint8_t> BuildRefs() {
    RefBuilder b;
    b.u32(3);   // count
    b.u32(0);

    // Real on-disk tag: the GUID words are byte-swapped, so this does not
    // read as the ASCII string it spells.
    uint8_t tex[16] = { 0x54,0x58,0x45,0x54, 0x00,0x45,0x52,0x55 };
    // Hash 0xB9A035976FA13B7B, stored as the two words the name spells.
    const uint32_t lo = 0xB9A03597, hi = 0x6FA13B7B;
    std::memcpy(tex + 8,  &lo, 4);
    std::memcpy(tex + 12, &hi, 4);
    b.entry(tex, "TX_athena10_body_gen_0d_B9A035976FA13B7B");

    uint8_t tex2[16] = { 0x54,0x58,0x45,0x54, 0x00,0x45,0x52,0x55 };
    const uint32_t lo2 = 0x2FDA9AAD, hi2 = 0x1BF0CED0;
    std::memcpy(tex2 + 8,  &lo2, 4);
    std::memcpy(tex2 + 12, &hi2, 4);
    b.entry(tex2, "TX_athena10_body_gen_0n_2FDA9AAD1BF0CED0");

    // A shader: an ordinary GUID, and the trailing name field runs short.
    uint8_t sh[16] = { 0x86,0x8e,0xbc,0x0c, 0x5f,0xed,0xff,0x5b,
                       0x70,0x2a,0x07,0x7b, 0x06,0xe7,0x54,0x4f };
    b.entry(sh, "de674f96622453eb_ps_10000207", /*truncate=*/true);

    return b.buf;
}

std::vector<uint8_t> BuildMat() {
    std::vector<uint8_t> m(0x2B0, 0);
    const uint64_t hash = 0xDE674F96622453EBull;
    std::memcpy(m.data() + 0x10, &hash, 8);

    const uint32_t sections[7] = { 0xF0, 0x1E0, 0x240, 0x240, 0x240, 0x280, 0x30 };
    std::memcpy(m.data() + 0xC0, sections, sizeof(sections));

    const uint16_t counts[6] = { 2, 0, 0, 0, 0, 0 };
    std::memcpy(m.data() + 0xE0, counts, sizeof(counts));

    auto param = [&](size_t i, uint64_t nameHash, uint16_t code,
                     uint16_t cbOff, float v) {
        uint8_t* p = m.data() + 0xF0 + i * 24;
        std::memcpy(p, &nameHash, 8);
        std::memcpy(p + 16, &code, 2);
        std::memcpy(p + 18, &cbOff, 2);
        std::memcpy(p + 20, &v, 4);
    };
    param(0, 0xD9EDF5D53BDB036Eull, 0x0409, 0x0000, 1.0f);
    param(1, 0x1F69542200BEC98Eull, 0x0841, 0x0020, 255.0f);
    return m;
}

std::shared_ptr<Onyx::Vfs::MemoryFile> Mem(std::vector<uint8_t> v) {
    return std::make_shared<Onyx::Vfs::MemoryFile>(std::move(v));
}

} // namespace

TEST_CASE("Material parameters carry their constant-buffer offsets") {
    Onyx::GOWRMaterial mat;
    REQUIRE(Onyx::GOWRMaterialParse(Mem(BuildMat()), nullptr, mat));

    CHECK(mat.hash == 0xDE674F96622453EBull);
    REQUIRE(mat.params.size() == 2);
    CHECK(mat.params[0].nameHash == 0xD9EDF5D53BDB036Eull);
    CHECK(mat.params[0].cbufferOffset == 0x0000);
    CHECK(mat.params[1].cbufferOffset == 0x0020);
    CHECK(mat.params[1].value == doctest::Approx(255.0f));
}

TEST_CASE("A reference is a texture because of its GUID, not its name") {
    std::vector<Onyx::MatReference> refs;
    REQUIRE(Onyx::GOWRMaterialParseRefs(Mem(BuildRefs()), refs));
    REQUIRE(refs.size() == 3);

    CHECK(refs[0].isTexture);
    CHECK_FALSE(refs[0].isShader);
    CHECK(refs[0].textureHash == 0xB9A035976FA13B7Bull);

    CHECK(refs[2].isShader);
    CHECK_FALSE(refs[2].isTexture);
}

TEST_CASE("The texpack key comes from the name, not the GUID") {
    // The two disagree often: across r_heroa00's 8396 texture references the
    // GUID carries a different hash than the name 937 times, and it is the name
    // hash the texpack is keyed by - 2772 of that WAD's 2798 textures resolve
    // through it.
    RefBuilder b;
    b.u32(1);
    b.u32(0);
    uint8_t g[16] = { 0x54,0x58,0x45,0x54, 0x00,0x45,0x52,0x55 };
    const uint32_t lo = 0x54EABE56, hi = 0x31989ADE;   // deliberately not the name
    std::memcpy(g + 8,  &lo, 4);
    std::memcpy(g + 12, &hi, 4);
    b.entry(g, "TX_mm_rock_lava_01_gen_0d_9844A99EBCFCFFA1");

    std::vector<Onyx::MatReference> refs;
    REQUIRE(Onyx::GOWRMaterialParseRefs(Mem(b.buf), refs));
    REQUIRE(refs.size() == 1);
    CHECK(refs[0].textureHash == 0x9844A99EBCFCFFA1ull);
}

TEST_CASE("A name with no hash falls back to the GUID") {
    RefBuilder b;
    b.u32(1);
    b.u32(0);
    uint8_t g[16] = { 0x54,0x58,0x45,0x54, 0x00,0x45,0x52,0x55 };
    const uint32_t lo = 0xAABBCCDD, hi = 0x11223344;
    std::memcpy(g + 8,  &lo, 4);
    std::memcpy(g + 12, &hi, 4);
    b.entry(g, "TX_no_hash_here");

    std::vector<Onyx::MatReference> refs;
    REQUIRE(Onyx::GOWRMaterialParseRefs(Mem(b.buf), refs));
    CHECK(refs[0].textureHash == 0xAABBCCDD11223344ull);
}

TEST_CASE("Texture roles come from the channel suffix") {
    std::vector<Onyx::MatReference> refs;
    REQUIRE(Onyx::GOWRMaterialParseRefs(Mem(BuildRefs()), refs));
    CHECK(refs[0].role == Onyx::TextureRole::Diffuse);
    CHECK(refs[1].role == Onyx::TextureRole::Normal);
}

TEST_CASE("Two-letter channel suffixes parse as their own roles") {
    // sc and sd are two characters where every other suffix is one, so a
    // parser that assumes a single letter silently files them as Unknown -
    // and they are common: gloss, scatter and detail account for 152 of the
    // 1148 textures across the shipped character WADs.
    RefBuilder b;
    b.u32(4);
    b.u32(0);

    auto tex = [&](const char* name) {
        uint8_t g[16] = { 0x54,0x58,0x45,0x54, 0x00,0x45,0x52,0x55 };
        b.entry(g, name);
    };
    tex("TX_atreus_body_gen_0g_1111111122222222");
    tex("TX_atreus_body_gen_0sc_3333333344444444");
    tex("TX_atreus_body_gen_0sd_5555555566666666");
    tex("TX_atreus_body_gen_0ao_7777777788888888");

    std::vector<Onyx::MatReference> refs;
    REQUIRE(Onyx::GOWRMaterialParseRefs(Mem(b.buf), refs));
    REQUIRE(refs.size() == 4);
    CHECK(refs[0].role == Onyx::TextureRole::Gloss);
    CHECK(refs[1].role == Onyx::TextureRole::Scatter);
    CHECK(refs[2].role == Onyx::TextureRole::Detail);
    CHECK(refs[3].role == Onyx::TextureRole::AmbientOcclusion);
}

TEST_CASE("Both channel-naming conventions are read") {
    // Assets spell the channel either as _0d_ or as plain _d_. Reading only the
    // first form makes a material look like it declares no diffuse. "gen" sits
    // in the same position as a tag but is a literal word, so it must not be
    // mistaken for one.
    RefBuilder b;
    b.u32(4);
    b.u32(0);
    auto tex = [&](const char* name) {
        uint8_t g[16] = { 0x54,0x58,0x45,0x54, 0x00,0x45,0x52,0x55 };
        b.entry(g, name);
    };
    tex("TX_standardeye00_lacrimal_d_2C0DD348EE2EF243");
    tex("TX_atreus00_eye_lacrimal_o_FA119503F5688267");
    tex("TX_kratos_body_nm_1111111122222222");
    tex("TX_death01_112E6D2408347E77");

    std::vector<Onyx::MatReference> refs;
    REQUIRE(Onyx::GOWRMaterialParseRefs(Mem(b.buf), refs));
    REQUIRE(refs.size() == 4);
    CHECK(refs[0].role == Onyx::TextureRole::Diffuse);
    CHECK(refs[1].role == Onyx::TextureRole::AmbientOcclusion);
    CHECK(refs[2].role == Onyx::TextureRole::Normal);
    CHECK(refs[3].role == Onyx::TextureRole::Unknown);   // no channel at all
}

TEST_CASE("A gen infix is not mistaken for a channel") {
    RefBuilder b;
    b.u32(1);
    b.u32(0);
    uint8_t g[16] = { 0x54,0x58,0x45,0x54, 0x00,0x45,0x52,0x55 };
    b.entry(g, "TX_cloudtile_gen_DCB6E128A9BEF8A5");
    std::vector<Onyx::MatReference> refs;
    REQUIRE(Onyx::GOWRMaterialParseRefs(Mem(b.buf), refs));
    CHECK(refs[0].role == Onyx::TextureRole::Unknown);
}

TEST_CASE("A truncated trailing reference is still read") {
    std::vector<Onyx::MatReference> refs;
    REQUIRE(Onyx::GOWRMaterialParseRefs(Mem(BuildRefs()), refs));
    REQUIRE(refs.size() == 3);
    CHECK(refs[2].name == "de674f96622453eb_ps_10000207");
}

TEST_CASE("Lookups by role and kind") {
    Onyx::GOWRMaterial mat;
    REQUIRE(Onyx::GOWRMaterialParse(Mem(BuildMat()), Mem(BuildRefs()), mat));

    CHECK(mat.Textures().size() == 2);
    CHECK(mat.Shaders().size() == 1);

    const auto* diffuse = mat.Texture(Onyx::TextureRole::Diffuse);
    REQUIRE(diffuse != nullptr);
    CHECK(diffuse->textureHash == 0xB9A035976FA13B7Bull);
    CHECK(mat.Texture(Onyx::TextureRole::Emissive) == nullptr);
}

// -- Naming a material's parameters from its shader ---------------------------
//
// The MAT entry stores each parameter as a hash and a byte offset; the string
// is nowhere in the file. The pixel shader's ConstBuf__materialData names a
// member at every one of those offsets, because the two are descriptions of
// the SAME buffer -- so the offset is a join key and the names come back.
//
// The layouts below are r_athena00's, read off the fixture WAD. Its three
// materials give the discriminating pair: two share a nine-member buffer, and
// the third inserts a layer_1__diffuse that pushes the floats along. A fixed
// table would pass the first and fail the second.

namespace {

// MAT_14EF18DD481D00DC and MAT_DEE24F0F7562F93B, whose parameter tables are
// identical. Widths come from the parameter's own typeCode, whose high byte is
// the size in bytes: 0x0409 uint(4), 0x0401 float(4), 0x0841 float2(8).
std::vector<Onyx::MatParam> AthenaParams() {
    auto p = [](uint16_t typeCode, uint16_t offset) {
        Onyx::MatParam m;
        m.typeCode      = typeCode;
        m.cbufferOffset = offset;
        return m;
    };
    return { p(0x0409, 0),  p(0x0409, 4),  p(0x0409, 8),
             p(0x0409, 12), p(0x0409, 16),
             p(0x0401, 20), p(0x0401, 24),
             p(0x0841, 32),                 // float2 -- 28 is its alignment pad
             p(0x0401, 40) };
}

std::vector<Onyx::ShaderConstant> AthenaConstants() {
    return { {0,  4, "material_particlespawn"},
             {4,  4, "layer_0__diffuse"},
             {8,  4, "layer_0__normal"},
             {12, 4, "layer_0__ao"},
             {16, 4, "layer_0__opacity"},
             {20, 4, "material_cst_ModifyDepthFadeStartDist"},
             {24, 4, "material_cst_ModifyDepthFadeEndDist"},
             {32, 8, "material_cst_ParticleSpawnReference"},
             {40, 4, "material_cst_ParticleSpawnReferenceRange"} };
}

} // namespace

TEST_CASE("Parameter names are recovered from the shader's constant buffer") {
    Onyx::GOWRMaterial mat;
    mat.hash   = 0x14EF18DD481D00DCull;
    mat.params = AthenaParams();

    CHECK(Onyx::NameParamsFromShaderConstants(mat, AthenaConstants()) == 9);
    CHECK(mat.params[1].name == "layer_0__diffuse");
    // The one the file could never have told us: a hash at offset 20.
    CHECK(mat.params[5].name == "material_cst_ModifyDepthFadeStartDist");
    // The float2 sits at 32, not 28, and the hole does not derail the join.
    CHECK(mat.params[7].name == "material_cst_ParticleSpawnReference");
}

TEST_CASE("The join follows the material, not a fixed layout") {
    // MAT_DE674F96622453EB's shader declares one more texture slot, and both
    // sides move together: layer_1__diffuse takes 20 and the two floats shift
    // from 20/24 to 24/28. Reading the first material's table onto this one
    // would call offset 24 ModifyDepthFadeEndDist when it is the START one.
    Onyx::GOWRMaterial mat;
    mat.hash = 0xDE674F96622453EBull;
    auto p = [](uint16_t typeCode, uint16_t offset) {
        Onyx::MatParam m;
        m.typeCode      = typeCode;
        m.cbufferOffset = offset;
        return m;
    };
    mat.params = { p(0x0409, 0),  p(0x0409, 4),  p(0x0409, 8),
                   p(0x0409, 12), p(0x0409, 16), p(0x0409, 20),
                   p(0x0401, 24), p(0x0401, 28),
                   p(0x0841, 32), p(0x0401, 40) };

    const std::vector<Onyx::ShaderConstant> constants = {
        {0,  4, "material_particlespawn"},
        {4,  4, "layer_0__diffuse"},
        {8,  4, "layer_0__normal"},
        {12, 4, "layer_0__ao"},
        {16, 4, "layer_0__opacity"},
        {20, 4, "layer_1__diffuse"},
        {24, 4, "material_cst_ModifyDepthFadeStartDist"},
        {28, 4, "material_cst_ModifyDepthFadeEndDist"},
        {32, 8, "material_cst_ParticleSpawnReference"},
        {40, 4, "material_cst_ParticleSpawnReferenceRange"} };

    CHECK(Onyx::NameParamsFromShaderConstants(mat, constants) == 10);
    CHECK(mat.params[5].name == "layer_1__diffuse");
    CHECK(mat.params[6].name == "material_cst_ModifyDepthFadeStartDist");
    CHECK(mat.params[7].name == "material_cst_ModifyDepthFadeEndDist");
}

TEST_CASE("One unmatched offset abandons the whole material") {
    // Eight of the nine would still land. They are dropped anyway: the two
    // sides describe one buffer, so a partial hit means the model is wrong,
    // and a table of eight confident names plus one blank invites trusting it.
    Onyx::GOWRMaterial mat;
    mat.params = AthenaParams();
    mat.params[3].cbufferOffset = 13;   // no member sits there

    CHECK(Onyx::NameParamsFromShaderConstants(mat, AthenaConstants()) == 0);
    for (const auto& p : mat.params) CHECK(p.name.empty());
}

TEST_CASE("A width the two sides disagree on abandons the material") {
    // Offset 32 is eight bytes to the parameter table and four to the shader.
    // That is two different layouts of the same buffer, which is exactly the
    // trap the smschema join fell into, so nothing is named.
    Onyx::GOWRMaterial mat;
    mat.params = AthenaParams();
    auto constants = AthenaConstants();
    constants[7].width = 4;

    CHECK(Onyx::NameParamsFromShaderConstants(mat, constants) == 0);
}

TEST_CASE("A width the shader cannot state is not held against the join") {
    // SlotWidth returns 0 for a spelling it does not recognise -- a matrix, a
    // nested struct. That is "cannot check", not "does not match", and the
    // offset alone still carries the name.
    Onyx::GOWRMaterial mat;
    mat.params = AthenaParams();
    auto constants = AthenaConstants();
    constants[7].width = 0;

    CHECK(Onyx::NameParamsFromShaderConstants(mat, constants) == 9);
    CHECK(mat.params[7].name == "material_cst_ParticleSpawnReference");
}
