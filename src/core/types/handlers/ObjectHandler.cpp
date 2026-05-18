// Object handler — GOW2 skeleton/joints container
// Magic: 0x00010001 (GOW2), 0x00040001 (GOW1)
//
// Resolution follows the Go project (god_of_war_browser):
//   Object iterates children by type (Model, Collision, Animation...)
//   If a child is a reference (no children), resolve by exact name in WAD.

#include "core/types/TypeRegistry.h"
#include "core/interfaces/IGameProfile.h"
#include "core/types/ITypeHandler.h"
#include "core/parsers/gow2/ObjectParser.h"
#include "core/parsers/gow2/MeshParser.h"
#include "core/parsers/gow2/MaterialParser.h"
#include "core/parsers/gow2/TextureParser.h"
#include "core/parsers/gow2/AnimationParser.h"
#include "core/parsers/shared/SceneNode.h"
#include "core/parsers/shared/ScriptTargetParser.h"
#include "core/vfs/SliceFile.h"
#include "core/Logger.h"
#include "ui/viewers/Viewport3D.h"
#include <cstring>
#include <functional>
#include "fonts/SFSymbols.h"

namespace {

// ── Resolve reference: find a node with exact name + type that has children ──
// Mirrors Go's GetNodeByName: when a child node is a reference (no children/no payload),
// the real definition with the same name exists elsewhere in the WAD tree.
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

// Same but for payload (size > 0) instead of children
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

// Find texture by exact name (Material → Texture uses name lookup, same as Go)
static const ParsedEntry* FindTexture(const std::vector<ParsedEntry>& nodes, const std::string& name) {
    for (const auto& c : nodes) {
        if (c.typeId == GOW::TypeId::Texture && c.name == name) return &c;
        if (auto f = FindTexture(c.children, name)) return f;
    }
    return nullptr;
}

// ── Select main texture layer (same priority as Go: StrangeBlended > Usual > first) ──
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

// ── Process a single Model node: extract meshes + materials ─────────────────
static void ProcessModel(const ParsedEntry& model, OpenWad& wad,
                          GOW::SceneData& scene) {
    std::vector<const ParsedEntry*> meshSources;
    std::vector<const ParsedEntry*> matEntries;
    bool isModelSky = false;

    // Iterate children by type (like Go's mdl.Marshal iterating SubGroupNodes)
    for (const auto& child : model.children) {
        if (child.typeId == GOW::TypeId::Mesh && child.size > 0) {
            meshSources.push_back(&child);
        } else if (child.typeId == GOW::TypeId::Material || (child.size == 0 && child.typeId == GOW::TypeId::Unknown)) {
            const ParsedEntry* mat = &child;
            // Material reference? Resolve by exact name
            if (mat->size == 0) {
                if (auto real = ResolvePayload(wad.entries, mat->name, GOW::TypeId::Material)) {
                    mat = real;
                    matEntries.push_back(mat);
                } else {
                    LOG_WARN("[ProcessModel] Could not resolve zero-sized material reference: '%s'", mat->name.c_str());
                }
            } else if (child.typeId == GOW::TypeId::Material) {
                matEntries.push_back(mat);
            }
        } else if (child.typeId == GOW::TypeId::Script && child.size > 0) {
            std::string target = GOW::ScriptTargetParser::ExtractTargetName(child, wad.fileSource);
            if (target == "SCR_Sky") {
                isModelSky = true;
                LOG_INFO("[ProcessModel] Found SCR_Sky on model '%s', marking as sky", model.name.c_str());
            }
        }
    }

    uint32_t materialOffset = scene.materials.size();

    LOG_INFO("[ProcessModel] Model '%s': %zu mesh children, %zu material children, materialOffset=%u",
             model.name.c_str(), meshSources.size(), matEntries.size(), materialOffset);

    // Parse materials → MaterialInfo (store all layers for main layer selection)
    for (size_t mi = 0; mi < matEntries.size(); ++mi) {
        const auto* mat = matEntries[mi];
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
        LOG_INFO("[ProcessModel]   mat[%zu] = '%s', layers=%zu, mainLayer='%s'",
                 mi + materialOffset, mat->name.c_str(), matInfo.layers.size(),
                 matInfo.layers.empty() ? "(none)" : matInfo.layers[0].textureName.c_str());
        scene.materials.push_back(std::move(matInfo));
    }

    // Parse mesh geometry
    for (const auto* src : meshSources) {
        GOW::SliceFile slice(wad.fileSource, src->offset, src->size);
        if (auto data = GOW::GOW2MeshParser::Parse(slice, 0, src->size)) {
            for (auto& p : data->parts) {
                LOG_INFO("[ProcessModel]   part '%s' materialId=%d (raw) → %d (offset)",
                         p.name.c_str(), p.materialId, p.materialId + (int)materialOffset);
                p.materialId += materialOffset;
                p.isSky = isModelSky;
                scene.meshParts.push_back(std::move(p));
            }
        }
    }
}

// ── Build SceneData from Object entry ──────────────────────────────────────

static std::unique_ptr<GOW::SceneData> BuildSceneFromObjectEntry(
    const ParsedEntry& entry, OpenWad& wad, uint32_t magic)
{
    if (!wad.fileSource) return nullptr;
    auto scene = std::make_unique<GOW::SceneData>();

    // 1. Parse skeleton from Object payload
    if (entry.size > 0) {
        std::vector<uint8_t> objBuf(entry.size);
        GOW::SliceFile slice(wad.fileSource, entry.offset, entry.size);
        slice.Seek(0, SEEK_SET);
        slice.Read(objBuf.data(), entry.size);
        scene->skeleton = std::shared_ptr<GOW::ObjectData>(
            GOW::GOW2ObjectParser::Parse(objBuf.data(), entry.size, magic).release());
    }

    // 2. Iterate children by type (like Go's obj.Marshal iterating SubGroupNodes)
    //    If a Model child is a reference (no children), resolve by exact name in WAD.
    for (const auto& child : entry.children) {
        if (child.typeId == GOW::TypeId::Model) {
            const ParsedEntry* model = &child;
            if (model->children.empty()) {
                // Reference node — resolve definition by exact name
                if (auto resolved = ResolveRef(wad.entries, child.name, GOW::TypeId::Model))
                    model = resolved;
            }
            if (!model->children.empty()) {
                ProcessModel(*model, wad, *scene);
            }
        }
        // 2b. Parse Animation child (like Go's obj.Marshal case *anm.Animations)
        else if (child.typeId == GOW::TypeId::Animation && child.size > 0) {
            GOW::SliceFile slice(wad.fileSource, child.offset, child.size);
            std::vector<uint8_t> anmBuf(child.size);
            slice.Seek(0, SEEK_SET);
            slice.Read(anmBuf.data(), child.size);
            auto animData = GOW::GOW2AnimationParser::Parse(anmBuf.data(), child.size);
            if (animData) {
                scene->animations = std::shared_ptr<GOW::AnimationData>(animData.release());
                LOG_INFO("[ObjectHandler] Parsed animation '%s': %d groups, %d acts",
                         child.name.c_str(), (int)scene->animations->groups.size(),
                         scene->animations->TotalActs());
            }
        }
    }

    if (scene->IsEmpty()) {
        // Many nodes are purely logical (triggers, collision, sound, cameras) and will have no meshes.
        LOG_INFO("[ObjectHandler] No meshes found for object '%s' (Expected for logical/trigger nodes)", entry.name.c_str());
        return scene;
    }

    // 3. Resolve textures: select main layer per material (like Go: 1 texture per material)
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

    LOG_INFO("[ObjectHandler] Built SceneData: %zu parts, %zu materials, %zu textures, skeleton=%s",
             scene->meshParts.size(), scene->materials.size(), scene->textures.size(),
             scene->HasSkeleton() ? "yes" : "no");

    return scene;
}

// ── Handler classes ────────────────────────────────────────────────────────

class ObjectHandlerGOW2 : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::Object; }
    const char*  GetName()  const override { return "Object"; }
    uint32_t     GetMagic() const override { return 0x00010001; }
    const char*  GetIcon()  const override { return ICON_SF_CUBE_FILL; }
    Color4f      GetColor() const override { return {0.55f, 0.9f, 1.0f, 1.0f}; }

    std::unique_ptr<GOW::SceneData> BuildSceneData(const ParsedEntry& entry, OpenWad& wad) override {
        uint32_t actualMagic = 0;
        if (wad.fileSource) {
            wad.fileSource->Seek(entry.offset, SEEK_SET);
            wad.fileSource->Read(&actualMagic, 4);
        }
        return BuildSceneFromObjectEntry(entry, wad, actualMagic);
    }

    std::shared_ptr<GOW::IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override {
        uint32_t actualMagic = 0;
        if (wad.fileSource) {
            wad.fileSource->Seek(entry.offset, SEEK_SET);
            wad.fileSource->Read(&actualMagic, 4);
        }
        auto scene = BuildSceneFromObjectEntry(entry, wad, actualMagic);
        auto vp = std::make_shared<GOW::Viewport3D>(entry.name);
        if (scene && !scene->IsEmpty()) {
            vp->LoadScene(std::move(scene));
        }
        return vp;
    }
};

} // anonymous namespace

REGISTER_TYPE(GOW2, ObjectHandlerGOW2);
