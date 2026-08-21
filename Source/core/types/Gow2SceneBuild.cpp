#include "core/types/Gow2SceneBuild.h"

#include "core/parsers/gow2/MaterialParser.h"
#include "core/parsers/gow2/TextureParser.h"

#include <Onyx/Services/Logger.h>
#include <Onyx/Types/TypeCatalog.h>
#include "core/types/GameTypes.h"

#include <cstring>

namespace Onyx::Gow2 {

// The layer a GOW2 material actually draws, in the Go reference's priority
// order: StrangeBlended > Usual > first.
//
// The pre-port code ran this selection TWICE -- once to swap the winner into
// layers[0] (so the renderer, which read layers[textureLayer] with
// textureLayer pinned to 0, would sample it) and again to pick the texture.
// Because the "Usual" branch keeps scanning after a match, it returns the
// LAST qualifying layer, so re-running it over the already-swapped vector
// could land on a different layer than the swap had chosen. v1.1 retires that
// first half outright -- MeshPart::textureLayer "survives only as a
// blend-ordering hint ... it no longer indexes into a material's texture
// layers" (Onyx/Parsers/SceneNode.h) -- so the reorder has no consumer left
// and only one selection remains. For a material with zero or one
// texture-bearing layer, which is the overwhelming majority, both spellings
// agree.
const GOW2MaterialParser::MaterialLayer*
SelectMainLayer(const std::vector<GOW2MaterialParser::MaterialLayer>& layers) {
    const GOW2MaterialParser::MaterialLayer* main = nullptr;
    for (const auto& layer : layers) {
        if (layer.renderingMethod == 3) {
            return &layer;              // StrangeBlended -- highest priority
        } else if (layer.renderingMethod == 0 && layer.hasTexture) {
            main = &layer;              // Usual -- second priority
        } else if (!main) {
            main = &layer;              // first layer -- fallback
        }
    }
    return main;
}

namespace {

Onyx::Parsers::BlendMode BlendModeOf(int renderingMethod) {
    switch (renderingMethod) {
        case 1:  return Onyx::Parsers::BlendMode::Additive;
        case 2:  return Onyx::Parsers::BlendMode::Subtractive;
        case 3:  return Onyx::Parsers::BlendMode::EnvMap;
        default: return Onyx::Parsers::BlendMode::Normal;
    }
}

} // namespace

// ── SceneTypes ──────────────────────────────────────────────────────────────

// Looked up fresh per construction rather than cached in a static: neither
// Gow2Module::RegisterTypes() nor GameTypes::RegisterGameTypes() is
// guaranteed to have run the first time a handler is reached (registration
// order is a composition-root concern), and a stale "not found" cached from
// too early a call would stick forever. Both lookups are hash-map hits, and
// this is built per scene build, not per frame.
SceneTypes::SceneTypes() {
    auto& cat = Onyx::Types::TypeCatalog::Get();
    object    = {cat.Find("gow2.object"),    Onyx::GameTypes::Object};
    model     = {cat.Find("gow2.model"),     Onyx::GameTypes::Model};
    mesh      = {cat.Find("gow2.mesh"),      Onyx::GameTypes::Mesh};
    material  = {cat.Find("gow2.material"),  Onyx::GameTypes::Material};
    texture   = {cat.Find("gow2.texture"),   Onyx::GameTypes::Texture};
    script    = {cat.Find("gow2.script"),    Onyx::GameTypes::Script};
    animation = {cat.Find("gow2.animation"), Onyx::GameTypes::Animation};
    unknown   = {cat.Find("gow2.unknown"),   Onyx::GameTypes::Unknown};
}

bool SceneTypes::Valid() const {
    return model.Known() && mesh.Known() && material.Known() && texture.Known();
}

// ── Tree resolution ─────────────────────────────────────────────────────────

const Onyx::Domain::AssetEntry* ResolveRef(const std::vector<Onyx::Domain::AssetEntry>& tree,
                                           const std::string& name,
                                           const TypeAlt& type) {
    for (const auto& n : tree) {
        if (type.Matches(n.typeId) && n.name == name && !n.children.empty())
            return &n;
        if (auto found = ResolveRef(n.children, name, type))
            return found;
    }
    return nullptr;
}

const Onyx::Domain::AssetEntry* ResolvePayload(const std::vector<Onyx::Domain::AssetEntry>& tree,
                                               const std::string& name,
                                               const TypeAlt& type) {
    for (const auto& n : tree) {
        if (type.Matches(n.typeId) && n.name == name && n.source.size > 0)
            return &n;
        if (auto found = ResolvePayload(n.children, name, type))
            return found;
    }
    return nullptr;
}

const Onyx::Domain::AssetEntry* FindTexture(const std::vector<Onyx::Domain::AssetEntry>& nodes,
                                            const std::string& name,
                                            const SceneTypes& types) {
    for (const auto& c : nodes) {
        if (types.texture.Matches(c.typeId) && c.name == name) return &c;
        if (auto f = FindTexture(c.children, name, types)) return f;
    }
    return nullptr;
}

// ── Material staging ────────────────────────────────────────────────────────

std::string StageMaterial(const Onyx::Domain::AssetEntry& matEntry,
                          const std::shared_ptr<Onyx::Vfs::IFile>& file,
                          Onyx::Parsers::SceneData& scene) {
    Onyx::Parsers::MaterialDesc desc;
    std::string textureName;

    if (auto matData = GOW2MaterialParser::Parse(matEntry, file)) {
        std::memcpy(desc.baseColor, matData->baseColor, sizeof(float) * 4);

        if (const auto* main = SelectMainLayer(matData->layers)) {
            desc.blendMode = BlendModeOf(main->renderingMethod);
            std::memcpy(desc.blendColor, main->blendColor, sizeof(float) * 4);
            if (main->hasTexture) textureName = main->textureName;
        }

        ONYX_LOGF_INFO("[Gow2SceneBuild] mat[%zu] '%s': %zu layers, main tex='%s'",
                       scene.materials.size(), matEntry.name.c_str(),
                       matData->layers.size(),
                       textureName.empty() ? "(none)" : textureName.c_str());
    } else {
        ONYX_LOGF_WARN("[Gow2SceneBuild] material '%s' failed to parse -- staged untextured",
                       matEntry.name.c_str());
    }

    scene.materials.push_back(std::move(desc));
    return textureName;
}

void BindDiffuse(const std::string& textureName,
                 size_t materialIndex,
                 Onyx::Domain::AssetContainer& wad,
                 const SceneTypes& types,
                 Onyx::Parsers::SceneData& scene) {
    if (textureName.empty() || materialIndex >= scene.materials.size()) return;

    const auto* texEntry = FindTexture(wad.entries, textureName, types);
    if (!texEntry) {
        ONYX_LOGF_WARN("[Gow2SceneBuild] material[%zu] names texture '%s', absent from this WAD",
                       materialIndex, textureName.c_str());
        return;
    }

    auto texData = GOW2TextureParser::Parse(*texEntry, wad.entries, wad.fileSource);
    if (!texData) {
        ONYX_LOGF_WARN("[Gow2SceneBuild] texture '%s' failed to decode", textureName.c_str());
        return;
    }

    // The role's value is the slot in the flat pool, so push first and bind
    // the index the push produced.
    scene.textures.push_back(std::move(texData));
    scene.materials[materialIndex].textures[Onyx::Parsers::TextureRole::Diffuse] =
        static_cast<int>(scene.textures.size() - 1);
}

} // namespace Onyx::Gow2
