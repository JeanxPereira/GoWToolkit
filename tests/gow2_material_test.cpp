// Pins the two rules Phase 3 moved onto Onyx v1.1's material model.
//
// Neither rule had a test before, and one of them changed: the pre-port code
// selected a GOW2 material's main layer twice (once to swap it into index 0
// for a renderer that read layers[textureLayer], once to pick the texture),
// and v1.1 retired the first half by turning MeshPart::textureLayer into a
// blend-ordering hint. The surviving single selection is asserted here so the
// change is witnessed rather than assumed.

#include <doctest/doctest.h>

#include "core/types/Gow2SceneBuild.h"
#include "core/types/TextureRoles.h"

using Onyx::GOW2MaterialParser;
using Layer = GOW2MaterialParser::MaterialLayer;

namespace {

Layer MakeLayer(int renderingMethod, bool hasTexture, const char* name) {
    Layer l{};
    l.renderingMethod = renderingMethod;
    l.hasTexture      = hasTexture;
    l.textureName     = name;
    return l;
}

} // namespace

TEST_CASE("SelectMainLayer: no layers selects nothing") {
    std::vector<Layer> layers;
    CHECK(Onyx::Gow2::SelectMainLayer(layers) == nullptr);
}

TEST_CASE("SelectMainLayer: a lone layer wins regardless of method") {
    std::vector<Layer> layers{MakeLayer(1, false, "additive_no_tex")};
    const auto* main = Onyx::Gow2::SelectMainLayer(layers);
    REQUIRE(main != nullptr);
    CHECK(main->textureName == "additive_no_tex");
}

TEST_CASE("SelectMainLayer: StrangeBlended outranks a textured Usual layer") {
    // Method 3 (EnvMap/StrangeBlended) is the Go reference's top priority and
    // returns immediately -- even when a later Usual layer carries a texture.
    std::vector<Layer> layers{
        MakeLayer(0, true, "usual_with_tex"),
        MakeLayer(3, true, "strange_blended"),
        MakeLayer(0, true, "later_usual"),
    };
    const auto* main = Onyx::Gow2::SelectMainLayer(layers);
    REQUIRE(main != nullptr);
    CHECK(main->textureName == "strange_blended");
}

TEST_CASE("SelectMainLayer: a textured Usual layer outranks the first layer") {
    std::vector<Layer> layers{
        MakeLayer(1, false, "additive_first"),
        MakeLayer(0, true,  "usual_with_tex"),
    };
    const auto* main = Onyx::Gow2::SelectMainLayer(layers);
    REQUIRE(main != nullptr);
    CHECK(main->textureName == "usual_with_tex");
}

TEST_CASE("SelectMainLayer: an untextured Usual layer does not outrank the first") {
    // hasTexture gates the Usual branch, so a method-0 layer with no texture
    // falls through to the first-layer fallback.
    std::vector<Layer> layers{
        MakeLayer(1, true,  "additive_first"),
        MakeLayer(0, false, "usual_no_tex"),
    };
    const auto* main = Onyx::Gow2::SelectMainLayer(layers);
    REQUIRE(main != nullptr);
    CHECK(main->textureName == "additive_first");
}

TEST_CASE("SelectMainLayer: with several textured Usual layers, the last one wins") {
    // This is the case where the retired double-selection could disagree with
    // itself: the loop keeps scanning after a match, so it returns the LAST
    // qualifying layer. The pre-port code then swapped that winner into index
    // 0 and re-ran the same scan over the swapped vector, which would pick a
    // different layer again. One selection, one answer.
    std::vector<Layer> layers{
        MakeLayer(0, true, "first_usual"),
        MakeLayer(0, true, "second_usual"),
        MakeLayer(0, true, "third_usual"),
    };
    const auto* main = Onyx::Gow2::SelectMainLayer(layers);
    REQUIRE(main != nullptr);
    CHECK(main->textureName == "third_usual");
}

// ── GOWR role bridge ────────────────────────────────────────────────────────

TEST_CASE("ToSceneRole maps every role the renderer can sample") {
    using G = Onyx::TextureRole;
    using S = Onyx::Parsers::TextureRole;

    CHECK(Onyx::ToSceneRole(G::Diffuse)          == S::Diffuse);
    CHECK(Onyx::ToSceneRole(G::Normal)           == S::Normal);
    CHECK(Onyx::ToSceneRole(G::AmbientOcclusion) == S::Occlusion);  // spelt differently
    CHECK(Onyx::ToSceneRole(G::Height)           == S::Height);
    CHECK(Onyx::ToSceneRole(G::Emissive)         == S::Emissive);
    CHECK(Onyx::ToSceneRole(G::Gloss)            == S::Gloss);
    CHECK(Onyx::ToSceneRole(G::Scatter)          == S::Scatter);
    CHECK(Onyx::ToSceneRole(G::Detail)           == S::Detail);
}

TEST_CASE("ToSceneRole rejects roles Onyx has no sampler for") {
    using G = Onyx::TextureRole;

    // GOWR materials declare these; Onyx::Parsers::TextureRole has no member
    // for them, so decoding one would upload a texture nothing reads.
    CHECK_FALSE(Onyx::ToSceneRole(G::Specular).has_value());
    CHECK_FALSE(Onyx::ToSceneRole(G::Roughness).has_value());
    CHECK_FALSE(Onyx::ToSceneRole(G::Metallic).has_value());
    CHECK_FALSE(Onyx::ToSceneRole(G::Unknown).has_value());
}

TEST_CASE("SceneRoleName covers every Onyx role") {
    using S = Onyx::Parsers::TextureRole;
    for (auto role : {S::Diffuse, S::Normal, S::Occlusion, S::Gloss, S::Height,
                      S::Scatter, S::Detail, S::Emissive, S::EnvMap}) {
        CHECK(std::string(Onyx::SceneRoleName(role)) != "?");
    }
}
