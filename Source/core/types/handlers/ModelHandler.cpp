// Model handler â€” GOW1/2 mesh container (mdl_*)
// Magic: 0x0002000F (MODEL_MAGIC in god_of_war_browser)
//
// Resolution follows the Go project (god_of_war_browser):
//   Model iterates children by type (Mesh, Material, Script...)
//   Material references resolved by exact name.
//   Texture resolved by exact name from Material layers.
// Uses SceneData pipeline for correct per-layer texture resolution.

#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Types/ITypeHandler.h>
#include "core/types/WadDispatch.h"
#include "core/types/GameTypes.h"
#include <Onyx/Schema/AssetReader.h>
#include "core/formats/GOW2ModelFormat.h"

#include <Onyx/Viewers/Viewport3D.h>
#include <Onyx/Domain/IAssetProfile.h>
#include "core/parsers/gow2/MeshParser.h"
#include "core/parsers/gow2/TextureParser.h"
#include "core/parsers/gow2/MaterialParser.h"
#include <Onyx/Parsers/SceneNode.h>
#include "core/parsers/gow2/ScriptTargetParser.h"
#include <Onyx/Vfs/SliceFile.h>
#include "core/WadTypes.h"
#include <Onyx/Services/Logger.h>
#include <cstring>
#include <functional>
#include <Onyx/Fonts/SFSymbols.h>

namespace {

// Resolve a reference node: find definition with exact name + type + has children
static const AssetEntry* ResolveRef(const std::vector<AssetEntry>& tree,
                                      const std::string& name, Onyx::Types::TypeId type) {
    for (const auto& n : tree) {
        if (n.typeId == type && n.name == name && !n.children.empty())
            return &n;
        if (auto found = ResolveRef(n.children, name, type))
            return found;
    }
    return nullptr;
}

// Resolve a reference node by payload (size > 0)
static const AssetEntry* ResolvePayload(const std::vector<AssetEntry>& tree,
                                          const std::string& name, Onyx::Types::TypeId type) {
    for (const auto& n : tree) {
        if (n.typeId == type && n.name == name && n.size > 0)
            return &n;
        if (auto found = ResolvePayload(n.children, name, type))
            return found;
    }
    return nullptr;
}

// Find texture by exact name (same as Go's GetNodeByName for textures)
static const AssetEntry* FindTexture(const std::vector<AssetEntry>& nodes, const std::string& name) {
    for (const auto& c : nodes) {
        if (c.typeId == Onyx::GameTypes::Texture && c.name == name) return &c;
        if (auto f = FindTexture(c.children, name)) return f;
    }
    return nullptr;
}

// Select main texture layer (same priority as Go: StrangeBlended > Usual > first)
static const Onyx::Parsers::MaterialInfo::Layer* SelectMainLayer(const Onyx::Parsers::MaterialInfo& mat) {
    const Onyx::Parsers::MaterialInfo::Layer* main = nullptr;
    for (const auto& layer : mat.layers) {
        if (layer.blendMode == Onyx::Parsers::BlendMode::EnvMap) {
            return &layer; // StrangeBlended = highest priority
        } else if (layer.blendMode == Onyx::Parsers::BlendMode::Normal && layer.hasTexture) {
            main = &layer; // Usual = second priority
        } else if (!main) {
            main = &layer; // First layer = fallback
        }
    }
    return main;
}

class ModelHandler : public Onyx::Gow::IWadTypeHandler {
public:
    Onyx::Types::TypeId  GetId()    const override { return Onyx::GameTypes::Model; }
    const char*  GetName()  const override { return "Model"; }
    uint32_t     GetMagic() const override { return 0x0002000F; }
    const char*  GetIcon()  const override { return ICON_SF_CUBE_FILL; }  // symbol-misc
    Color4f      GetColor() const override { return {0.4f, 0.8f, 1.0f, 1.0f}; }  // azul

    std::shared_ptr<Onyx::Schema::AssetNode> Parse(std::shared_ptr<Onyx::Vfs::IFile> file) override {
        if (!file || file->Size() < 24) return nullptr;
        Onyx::GOW2ModelFormat format;
        format.Initialize();
        return Onyx::Schema::AssetReader::Parse(*format.Root(), file);
    }

    std::unique_ptr<Onyx::Parsers::SceneData> BuildSceneData(const AssetEntry& entry, AssetContainer& wad) override {
        if (!wad.fileSource) return nullptr;

        // Resolve the Model entry itself â€” if it's a reference, find the definition
        const AssetEntry* model = &entry;
        if (model->children.empty()) {
            if (auto resolved = ResolveRef(wad.entries, entry.name, Onyx::GameTypes::Model))
                model = resolved;
        }
        if (model->children.empty()) return nullptr;

        // Build SceneData by iterating children (like Go's mdl.Marshal)
        auto scene = std::make_unique<Onyx::Parsers::SceneData>();

        std::vector<const AssetEntry*> meshSources;
        std::vector<const AssetEntry*> matEntries;

        for (const auto& child : model->children) {
            if (child.typeId == Onyx::GameTypes::Mesh && child.size > 0) {
                meshSources.push_back(&child);
            } else if (child.typeId == Onyx::GameTypes::Material) {
                const AssetEntry* mat = &child;
                if (mat->size == 0) {
                    if (auto real = ResolvePayload(wad.entries, mat->name, Onyx::GameTypes::Material))
                        mat = real;
                }
                matEntries.push_back(mat);
            }
        }

        uint32_t materialOffset = scene->materials.size();

        // Parse materials â†’ MaterialInfo (store all layers for main layer selection)
        for (const auto* mat : matEntries) {
            Onyx::Parsers::MaterialInfo matInfo;
            if (auto matData = Onyx::GOW2MaterialParser::Parse(*mat, wad.fileSource)) {
                std::memcpy(matInfo.baseColor, matData->baseColor, sizeof(float) * 4);
                for (const auto& layer : matData->layers) {
                    Onyx::Parsers::MaterialInfo::Layer li;
                    li.textureName = layer.textureName;
                    li.hasTexture = layer.hasTexture;
                    std::memcpy(li.blendColor, layer.blendColor, sizeof(float) * 4);
                    switch (layer.renderingMethod) {
                        case 1:  li.blendMode = Onyx::Parsers::BlendMode::Additive; break;
                        case 2:  li.blendMode = Onyx::Parsers::BlendMode::Subtractive; break;
                        case 3:  li.blendMode = Onyx::Parsers::BlendMode::EnvMap; break;
                        default: li.blendMode = Onyx::Parsers::BlendMode::Normal; break;
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
            Onyx::Vfs::SliceFile slice(wad.fileSource, src->offset, src->size);
            if (auto data = Onyx::GOW2MeshParser::Parse(slice, 0, src->size)) {
                for (auto& p : data->parts) {
                    p.materialId += materialOffset;
                    scene->meshParts.push_back(std::move(p));
                }
            }
        }

        if (scene->IsEmpty()) return nullptr;

        // Detect Sky script â€” flag both scene and individual parts so
        // SceneRenderer (which reads part.isSky into RenderBatch::isSky) can
        // route them through the sky pass.
        for (const auto& child : model->children) {
            if (child.typeId == Onyx::GameTypes::Script && child.size > 0) {
                std::string target = Onyx::Parsers::ScriptTargetParser::ExtractTargetName(child, wad.fileSource);
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
            std::vector<std::unique_ptr<Onyx::Parsers::TextureData>> matTextures;
            std::unique_ptr<Onyx::Parsers::TextureData> texData = nullptr;
            if (auto* mainLayer = SelectMainLayer(mat)) {
                if (mainLayer->hasTexture && !mainLayer->textureName.empty()) {
                    if (auto* texEntry = FindTexture(wad.entries, mainLayer->textureName)) {
                        texData = Onyx::GOW2TextureParser::Parse(*texEntry, wad.entries, wad.fileSource);
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

    std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        auto vp = std::make_shared<Onyx::Viewers::Viewport3D>(entry.name);
        if (auto scene = BuildSceneData(entry, wad)) {
            vp->LoadScene(std::move(scene));
        }
        return vp;
    }
};

} // anonymous namespace

REGISTER_GOW_TYPE(GOW2, ModelHandler);

