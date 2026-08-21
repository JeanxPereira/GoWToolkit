// Object handler — GOW2 skeleton/joints container
// Magic: 0x00010001 (GOW2), 0x00040001 (GOW1)
//
// Resolution follows the Go project (god_of_war_browser):
//   Object iterates children by type (Model, Collision, Animation...)
//   If a child is a reference (no children), resolve by exact name in WAD.

#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Types/ITypeHandler.h>
#include "core/types/WadDispatch.h"
#include "core/types/GameTypes.h"
#include "core/WadTypes.h"
#include "core/types/Gow2SceneBuild.h"
#include "core/parsers/gow2/ObjectParser.h"
#include "core/parsers/gow2/MeshParser.h"
#include "core/parsers/gow2/AnimationParser.h"
#include <Onyx/Parsers/SceneNode.h>
#include "core/parsers/gow2/ScriptTargetParser.h"
#include <Onyx/Vfs/SliceFile.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Viewers/Viewport3D.h>
#include <cstring>
#include <functional>
#include <Onyx/Fonts/SFSymbols.h>

namespace {

// ── Process a single Model node: extract meshes + materials ─────────────────
// Material texture names come back in `pendingTextures`, parallel to
// scene.materials, and are resolved in one pass after the caller knows the
// scene has geometry at all -- most GOW2 nodes are purely logical (triggers,
// collision, cameras) and decoding their textures would be wasted work.
static void ProcessModel(const AssetEntry& model, AssetContainer& wad,
                         const Onyx::Gow2::SceneTypes& types,
                         Onyx::Parsers::SceneData& scene,
                         std::vector<std::string>& pendingTextures) {
    std::vector<const AssetEntry*> meshSources;
    std::vector<const AssetEntry*> matEntries;
    bool isModelSky = false;

    // Iterate children by type (like Go's mdl.Marshal iterating SubGroupNodes)
    for (const auto& child : model.children) {
        if (child.typeId == types.mesh && child.source.size > 0) {
            meshSources.push_back(&child);
        } else if (child.typeId == types.material ||
                   (child.source.size == 0 && child.typeId == types.unknown)) {
            const AssetEntry* mat = &child;
            // Material reference? Resolve by exact name
            if (mat->source.size == 0) {
                if (auto real = Onyx::Gow2::ResolvePayload(wad.entries, mat->name, types.material)) {
                    mat = real;
                    matEntries.push_back(mat);
                } else {
                    ONYX_LOGF_WARN("[ProcessModel] Could not resolve zero-sized material reference: '%s'", mat->name.c_str());
                }
            } else if (child.typeId == types.material) {
                matEntries.push_back(mat);
            }
        } else if (child.typeId == types.script && child.source.size > 0) {
            std::string target = Onyx::Parsers::ScriptTargetParser::ExtractTargetName(child, wad.fileSource);
            if (target == "SCR_Sky") {
                isModelSky = true;
                ONYX_LOGF_INFO("[ProcessModel] Found SCR_Sky on model '%s', marking as sky", model.name.c_str());
            }
        }
    }

    uint32_t materialOffset = scene.materials.size();

    ONYX_LOGF_INFO("[ProcessModel] Model '%s': %zu mesh children, %zu material children, materialOffset=%u",
             model.name.c_str(), meshSources.size(), matEntries.size(), materialOffset);

    for (const auto* mat : matEntries)
        pendingTextures.push_back(Onyx::Gow2::StageMaterial(*mat, wad.fileSource, scene));

    // Parse mesh geometry
    for (const auto* src : meshSources) {
        Onyx::Vfs::SliceFile slice(wad.fileSource, src->source.offset, src->source.size);
        if (auto data = Onyx::GOW2MeshParser::Parse(slice, 0, src->source.size)) {
            for (auto& p : data->parts) {
                ONYX_LOGF_INFO("[ProcessModel]   part '%s' materialId=%d (raw) → %d (offset)",
                         p.name.c_str(), p.materialId, p.materialId + (int)materialOffset);
                p.materialId += materialOffset;
                p.isSky = isModelSky;
                scene.meshParts.push_back(std::move(p));
            }
        }
    }
}

// ── Build SceneData from Object entry ──────────────────────────────────────

static std::unique_ptr<Onyx::Parsers::SceneData> BuildSceneFromObjectEntry(
    const AssetEntry& entry, AssetContainer& wad, uint32_t magic)
{
    if (!wad.fileSource) return nullptr;
    auto scene = std::make_unique<Onyx::Parsers::SceneData>();

    // A real GOW2 tree carries Gow2Module's own minted ids, not the legacy
    // GameTypes externs -- see Gow2SceneBuild.h. Without them every type test
    // below is false and the walk silently produces nothing, so say so once
    // rather than returning a mysteriously empty scene.
    const Onyx::Gow2::SceneTypes types;
    if (!types.Valid()) {
        ONYX_LOGF_WARN("[ObjectHandler] Gow2Module types are not registered; "
                       "cannot walk '%s'", entry.name.c_str());
        return scene;
    }
    std::vector<std::string> pendingTextures;

    // 1. Parse skeleton from Object payload
    if (entry.source.size > 0) {
        std::vector<uint8_t> objBuf(entry.source.size);
        Onyx::Vfs::SliceFile slice(wad.fileSource, entry.source.offset, entry.source.size);
        slice.Seek(0, SEEK_SET);
        slice.Read(objBuf.data(), entry.source.size);
        scene->skeleton = std::shared_ptr<Onyx::Parsers::ObjectData>(
            Onyx::GOW2ObjectParser::Parse(objBuf.data(), entry.source.size, magic).release());
    }

    // 2. Iterate children by type (like Go's obj.Marshal iterating SubGroupNodes)
    //    If a Model child is a reference (no children), resolve by exact name in WAD.
    for (const auto& child : entry.children) {
        if (child.typeId == types.model) {
            const AssetEntry* model = &child;
            if (model->children.empty()) {
                // Reference node — resolve definition by exact name
                if (auto resolved = Onyx::Gow2::ResolveRef(wad.entries, child.name, types.model))
                    model = resolved;
            }
            if (!model->children.empty()) {
                ProcessModel(*model, wad, types, *scene, pendingTextures);
            }
        }
        // 2b. Parse Animation child (like Go's obj.Marshal case *anm.Animations)
        else if (child.typeId == types.animation && child.source.size > 0) {
            Onyx::Vfs::SliceFile slice(wad.fileSource, child.source.offset, child.source.size);
            std::vector<uint8_t> anmBuf(child.source.size);
            slice.Seek(0, SEEK_SET);
            slice.Read(anmBuf.data(), child.source.size);
            auto animData = Onyx::GOW2AnimationParser::Parse(anmBuf.data(), child.source.size);
            if (animData) {
                scene->animations = std::shared_ptr<Onyx::Parsers::AnimationData>(animData.release());
                ONYX_LOGF_INFO("[ObjectHandler] Parsed animation '%s': %d groups, %d acts",
                         child.name.c_str(), (int)scene->animations->groups.size(),
                         scene->animations->TotalActs());
            }
        }
    }

    if (scene->IsEmpty()) {
        // Many nodes are purely logical (triggers, collision, sound, cameras) and will have no meshes.
        ONYX_LOGF_INFO("[ObjectHandler] No meshes found for object '%s' (Expected for logical/trigger nodes)", entry.name.c_str());
        return scene;
    }

    // 3. Resolve textures: one per material, bound as Diffuse into the flat
    //    pool. A material whose texture is missing simply has no Diffuse role,
    //    which is v1.1's spelling of untextured -- the old nested container
    //    had to push an explicit null to mean the same thing.
    for (size_t mi = 0; mi < pendingTextures.size(); ++mi)
        Onyx::Gow2::BindDiffuse(pendingTextures[mi], mi, wad, types, *scene);

    ONYX_LOGF_INFO("[ObjectHandler] Built SceneData: %zu parts, %zu materials, %zu textures, skeleton=%s",
             scene->meshParts.size(), scene->materials.size(), scene->textures.size(),
             scene->HasSkeleton() ? "yes" : "no");

    return scene;
}

// ── Handler classes ────────────────────────────────────────────────────────

class ObjectHandlerGOW2 : public Onyx::Gow::IWadTypeHandler {
public:
    Onyx::Types::TypeId  GetId()    const override { return Onyx::GameTypes::Object; }
    const char*  GetName()  const override { return "Object"; }
    uint32_t     GetMagic() const override { return 0x00010001; }
    const char*  GetIcon()  const override { return ICON_SF_CUBE_FILL; }
    Color4f      GetColor() const override { return {0.55f, 0.9f, 1.0f, 1.0f}; }

    std::unique_ptr<Onyx::Parsers::SceneData> BuildSceneData(const AssetEntry& entry, AssetContainer& wad) override {
        uint32_t actualMagic = 0;
        if (wad.fileSource) {
            wad.fileSource->Seek(entry.source.offset, SEEK_SET);
            wad.fileSource->Read(&actualMagic, 4);
        }
        return BuildSceneFromObjectEntry(entry, wad, actualMagic);
    }

    std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        uint32_t actualMagic = 0;
        if (wad.fileSource) {
            wad.fileSource->Seek(entry.source.offset, SEEK_SET);
            wad.fileSource->Read(&actualMagic, 4);
        }
        auto scene = BuildSceneFromObjectEntry(entry, wad, actualMagic);
        auto vp = std::make_shared<Onyx::Viewers::Viewport3D>(entry.name);
        if (scene && !scene->IsEmpty()) {
            vp->LoadScene(std::move(scene));
        }
        return vp;
    }
};

} // anonymous namespace

REGISTER_GOW_TYPE(GOW2, ObjectHandlerGOW2);

