// Model handler — GOW1/2 mesh container (mdl_*)
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
#include "core/types/Gow2SceneBuild.h"
#include "core/parsers/gow2/MeshParser.h"
#include <Onyx/Parsers/SceneNode.h>
#include "core/parsers/gow2/ScriptTargetParser.h"
#include <Onyx/Vfs/SliceFile.h>
#include "core/WadTypes.h"
#include <Onyx/Services/Logger.h>
#include <cstring>
#include <functional>
#include <Onyx/Fonts/SFSymbols.h>

namespace {

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

        // A real GOW2 tree carries Gow2Module's own minted ids, not the legacy
        // GameTypes externs -- see Gow2SceneBuild.h. Without them nothing below
        // matches and the walk yields an empty scene for no visible reason.
        const Onyx::Gow2::SceneTypes types;
        if (!types.Valid()) {
            ONYX_LOGF_WARN("[ModelHandler] Gow2Module types are not registered; "
                           "cannot walk '%s'", entry.name.c_str());
            return nullptr;
        }

        // Resolve the Model entry itself — if it's a reference, find the definition
        const AssetEntry* model = &entry;
        if (model->children.empty()) {
            if (auto resolved = Onyx::Gow2::ResolveRef(wad.entries, entry.name, types.model))
                model = resolved;
        }
        if (model->children.empty()) return nullptr;

        // Build SceneData by iterating children (like Go's mdl.Marshal)
        auto scene = std::make_unique<Onyx::Parsers::SceneData>();

        std::vector<const AssetEntry*> meshSources;
        std::vector<const AssetEntry*> matEntries;

        for (const auto& child : model->children) {
            if (child.typeId == types.mesh && child.source.size > 0) {
                meshSources.push_back(&child);
            } else if (child.typeId == types.material) {
                const AssetEntry* mat = &child;
                if (mat->source.size == 0) {
                    if (auto real = Onyx::Gow2::ResolvePayload(wad.entries, mat->name, types.material))
                        mat = real;
                }
                matEntries.push_back(mat);
            }
        }

        uint32_t materialOffset = scene->materials.size();

        std::vector<std::string> pendingTextures;
        for (const auto* mat : matEntries)
            pendingTextures.push_back(Onyx::Gow2::StageMaterial(*mat, wad.fileSource, *scene));

        // Parse mesh geometry
        for (const auto* src : meshSources) {
            Onyx::Vfs::SliceFile slice(wad.fileSource, src->source.offset, src->source.size);
            if (auto data = Onyx::GOW2MeshParser::Parse(slice, 0, src->source.size)) {
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
            if (child.typeId == types.script && child.source.size > 0) {
                std::string target = Onyx::Parsers::ScriptTargetParser::ExtractTargetName(child, wad.fileSource);
                if (target == "SCR_Sky") {
                    scene->isSky = true;
                    for (auto& p : scene->meshParts) {
                        p.isSky = true;
                    }
                    ONYX_LOGF_INFO("[ModelHandler] Detected SCR_Sky in model '%s' (%zu parts flagged)",
                             model->name.c_str(), scene->meshParts.size());
                    break;
                }
            }
        }

        // Resolve textures: one per material, bound as Diffuse into the flat
        // pool. An unresolved texture leaves the role absent rather than
        // pushing a null, which is what v1.1 reads as untextured.
        for (size_t mi = 0; mi < pendingTextures.size(); ++mi)
            Onyx::Gow2::BindDiffuse(pendingTextures[mi], mi, wad, types, *scene);

        ONYX_LOGF_INFO("[ModelHandler] Built SceneData: %zu parts, %zu materials, %zu textures",
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

