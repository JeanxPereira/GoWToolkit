// Model handler — GOW1/2 mesh container (mdl_*)
// Magic: 0x0002000F (MODEL_MAGIC in god_of_war_browser)
//
// Resolution follows the Go project (god_of_war_browser):
//   Model iterates children by type (Mesh, Material, Script...)
//   Material references resolved by exact name.
//   Texture resolved by exact name from Material layers.
// Uses SceneData pipeline for correct per-layer texture resolution.

#include "core/types/TypeRegistry.h"
#include "core/types/ITypeHandler.h"
#include "core/schema/AssetReader.h"
#include "core/formats/GOW2ModelFormat.h"

#include "ui/viewers/Viewport3D.h"
#include "core/interfaces/IGameProfile.h"
#include "core/parsers/gow2/MeshParser.h"
#include "core/parsers/gow2/TextureParser.h"
#include "core/parsers/gow2/MaterialParser.h"
#include "core/parsers/shared/SceneNode.h"
#include "core/parsers/shared/ScriptTargetParser.h"
#include "core/vfs/SliceFile.h"
#include "core/WadTypes.h"
#include "core/Logger.h"
#include <cstring>
#include <functional>
#include "fonts/SFSymbols.h"

namespace {

// Resolve a reference node: find definition with exact name + type + has children
static const ParsedEntry* ResolveRef(const std::vector<ParsedEntry>& tree,
                                      const std::string& name, GOW::TypeId type) {
    for (const auto& n : tree) {
        if (n.typeId == type && n.name == name && !n.children.empty())
            return &n;
        if (auto found = ResolveRef(n.children, name, type))
            return found;
    }
    return nullptr;
}

// Resolve a reference node by payload (size > 0)
static const ParsedEntry* ResolvePayload(const std::vector<ParsedEntry>& tree,
                                          const std::string& name, GOW::TypeId type) {
    for (const auto& n : tree) {
        if (n.typeId == type && n.name == name && n.size > 0)
            return &n;
        if (auto found = ResolvePayload(n.children, name, type))
            return found;
    }
    return nullptr;
}

// Find texture by exact name (same as Go's GetNodeByName for textures)
static const ParsedEntry* FindTexture(const std::vector<ParsedEntry>& nodes, const std::string& name) {
    for (const auto& c : nodes) {
        if (c.typeId == GOW::TypeId::Texture && c.name == name) return &c;
        if (auto f = FindTexture(c.children, name)) return f;
    }
    return nullptr;
}

// Select main texture layer (same priority as Go: StrangeBlended > Usual > first)
static const GOW::MaterialInfo::Layer* SelectMainLayer(const GOW::MaterialInfo& mat) {
    const GOW::MaterialInfo::Layer* main = nullptr;
    for (const auto& layer : mat.layers) {
        if (layer.blendMode == GOW::BlendMode::EnvMap) {
            return &layer; // StrangeBlended = highest priority
        } else if (layer.blendMode == GOW::BlendMode::Normal && layer.hasTexture) {
            main = &layer; // Usual = second priority
        } else if (!main) {
            main = &layer; // First layer = fallback
        }
    }
    return main;
}

class ModelHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::Model; }
    const char*  GetName()  const override { return "Model"; }
    uint32_t     GetMagic() const override { return 0x0002000F; }
    const char*  GetIcon()  const override { return ICON_SF_CUBE_FILL; }  // symbol-misc
    Color4f      GetColor() const override { return {0.4f, 0.8f, 1.0f, 1.0f}; }  // azul

    std::shared_ptr<GOW::AssetNode> Parse(std::shared_ptr<GOW::IFile> file) override {
        if (!file || file->Size() < 24) return nullptr;
        GOW::GOW2ModelFormat format;
        format.Initialize();
        return GOW::AssetReader::Parse(*format.Root(), file);
    }

    std::unique_ptr<GOW::SceneData> BuildSceneData(const ParsedEntry& entry, OpenWad& wad) override {
        if (!wad.fileSource) return nullptr;

        // Resolve the Model entry itself — if it's a reference, find the definition
        const ParsedEntry* model = &entry;
        if (model->children.empty()) {
            if (auto resolved = ResolveRef(wad.entries, entry.name, GOW::TypeId::Model))
                model = resolved;
        }
        if (model->children.empty()) return nullptr;

        // Build SceneData by iterating children (like Go's mdl.Marshal)
        auto scene = std::make_unique<GOW::SceneData>();

        std::vector<const ParsedEntry*> meshSources;
        std::vector<const ParsedEntry*> matEntries;

        for (const auto& child : model->children) {
            if (child.typeId == GOW::TypeId::Mesh && child.size > 0) {
                meshSources.push_back(&child);
            } else if (child.typeId == GOW::TypeId::Material) {
                const ParsedEntry* mat = &child;
                if (mat->size == 0) {
                    if (auto real = ResolvePayload(wad.entries, mat->name, GOW::TypeId::Material))
                        mat = real;
                }
                matEntries.push_back(mat);
            }
        }

        uint32_t materialOffset = scene->materials.size();

        // Parse materials → MaterialInfo (store all layers for main layer selection)
        for (const auto* mat : matEntries) {
            GOW::MaterialInfo matInfo;
            if (auto matData = GOW::GOW2MaterialParser::Parse(*mat, wad.fileSource)) {
                std::memcpy(matInfo.baseColor, matData->baseColor, sizeof(float) * 4);
                for (const auto& layer : matData->layers) {
                    GOW::MaterialInfo::Layer li;
                    li.textureName = layer.textureName;
                    li.hasTexture = layer.hasTexture;
                    std::memcpy(li.blendColor, layer.blendColor, sizeof(float) * 4);
                    switch (layer.renderingMethod) {
                        case 1:  li.blendMode = GOW::BlendMode::Additive; break;
                        case 2:  li.blendMode = GOW::BlendMode::Subtractive; break;
                        case 3:  li.blendMode = GOW::BlendMode::EnvMap; break;
                        default: li.blendMode = GOW::BlendMode::Normal; break;
                    }
                    matInfo.layers.push_back(li);
                }
                // Reorder so main layer is at index 0 (renderer reads layers[textureLayer=0])
                if (matInfo.layers.size() > 1) {
                    if (auto* mainPtr = SelectMainLayer(matInfo)) {
                        size_t mainIdx = mainPtr - matInfo.layers.data();
                        if (mainIdx != 0) std::swap(matInfo.layers[0], matInfo.layers[mainIdx]);
                    }
                }
            }
            scene->materials.push_back(std::move(matInfo));
        }

        // Parse mesh geometry
        for (const auto* src : meshSources) {
            GOW::SliceFile slice(wad.fileSource, src->offset, src->size);
            if (auto data = GOW::GOW2MeshParser::Parse(slice, 0, src->size)) {
                for (auto& p : data->parts) {
                    p.materialId += materialOffset;
                    scene->meshParts.push_back(std::move(p));
                }
            }
        }

        if (scene->IsEmpty()) return nullptr;

        // Detect Sky script — flag both scene and individual parts so
        // SceneRenderer (which reads part.isSky into RenderBatch::isSky) can
        // route them through the sky pass.
        for (const auto& child : model->children) {
            if (child.typeId == GOW::TypeId::Script && child.size > 0) {
                std::string target = GOW::ScriptTargetParser::ExtractTargetName(child, wad.fileSource);
                if (target == "SCR_Sky") {
                    scene->isSky = true;
                    for (auto& p : scene->meshParts) {
                        p.isSky = true;
                    }
                    LOG_INFO("[ModelHandler] Detected SCR_Sky in model '%s' (%zu parts flagged)",
                             model->name.c_str(), scene->meshParts.size());
                    break;
                }
            }
        }

        // Resolve textures: select main layer per material (like Go: 1 texture per material)
        for (const auto& mat : scene->materials) {
            std::vector<std::unique_ptr<GOW::TextureData>> matTextures;
            std::unique_ptr<GOW::TextureData> texData = nullptr;
            if (auto* mainLayer = SelectMainLayer(mat)) {
                if (mainLayer->hasTexture && !mainLayer->textureName.empty()) {
                    if (auto* texEntry = FindTexture(wad.entries, mainLayer->textureName)) {
                        texData = GOW::GOW2TextureParser::Parse(*texEntry, wad.entries, wad.fileSource);
                    }
                }
            }
            matTextures.push_back(std::move(texData));
            scene->textures.push_back(std::move(matTextures));
        }

        LOG_INFO("[ModelHandler] Built SceneData: %zu parts, %zu materials, %zu textures",
                 scene->meshParts.size(), scene->materials.size(), scene->textures.size());

        return scene;
    }

    std::shared_ptr<GOW::IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override {
        auto vp = std::make_shared<GOW::Viewport3D>(entry.name);
        if (auto scene = BuildSceneData(entry, wad)) {
            vp->LoadScene(std::move(scene));
        }
        return vp;
    }
};

} // anonymous namespace

REGISTER_TYPE(GOW2, ModelHandler);
