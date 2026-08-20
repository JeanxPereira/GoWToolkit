#include "core/harness/AssetHarness.h"

#include <Onyx/Modules/Workspace.h>
#include <Onyx/Types/ITypeHandler.h>
#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Vfs/OsFile.h>

#include "core/modules/Gow2Module.h"
#include "core/modules/GowrModule.h"

#include <algorithm>
#include <memory>
#include <ostream>

namespace Onyx::Harness {

namespace {

// One Workspace, shared by every headless caller in this process, with both
// game modules registered exactly once. Onyx::Types::TypeCatalog::Get() is a
// process-wide singleton (see Gow2Module/GowrModule::RegisterTypes()), so a
// second Workspace instance here would just re-resolve the same module-
// namespaced keys to the same handles (TypeRegistrar::Add's "re-adding the
// same bare key returns the existing handle") -- this Meyer's singleton
// exists to avoid rebuilding the module set on every LoadContainer call, not
// to work around any registration hazard.
Onyx::Modules::Workspace& GetWorkspace() {
    static Onyx::Modules::Workspace workspace(Onyx::Types::TypeCatalog::Get());
    static bool modulesRegistered = [] {
        workspace.AddModule(std::make_unique<Onyx::Gowr::GowrModule>());
        workspace.AddModule(std::make_unique<Onyx::Gow2::Gow2Module>());
        return true;
    }();
    (void)modulesRegistered;
    return workspace;
}

} // namespace

const AssetEntry* FindEntryByName(const std::vector<AssetEntry>& entries,
                                  std::string_view name)
{
    for (const auto& e : entries) {
        if (e.name == name) return &e;
        if (auto* f = FindEntryByName(e.children, name)) return f;
    }
    return nullptr;
}

void CollectEntryNames(const std::vector<AssetEntry>& entries,
                       std::vector<std::string>& out)
{
    for (const auto& e : entries) {
        out.push_back(e.name);
        CollectEntryNames(e.children, out);
    }
}

bool LoadContainer(const LoadRequest& req, LoadResult& out)
{
    out.ok = false;

    if (!std::filesystem::is_regular_file(req.archive)) {
        out.error = "not a regular file: " + req.archive.string();
        return false;
    }

    // Module selection mirrors the old ProfileManager hint/detect split: an
    // explicit hint is passed straight through as Workspace::Open's
    // moduleHint (which itself wins outright over probing, never falling
    // back to it on a bad hint -- see Workspace::PrepareDocument); an empty
    // hint lets Workspace::Probe rank the registered modules by evidence
    // (RankProbes). Unlike the old harness, ISO input is no longer refused
    // here: Gow2Module declares a MountSpec for .iso (Phase 2 Task 3), so
    // Workspace::Open mounts and walks it the same way the GUI always could
    // -- this is new headless capability, not a port of existing behaviour.
    Onyx::Modules::Workspace& workspace = GetWorkspace();

    bool parsedOk = false;
    auto sub = workspace.Events().On<Onyx::Modules::TreeReady>(
        [&](const Onyx::Modules::TreeReady& e) { parsedOk = e.ok; });

    Onyx::Modules::DocumentId id = workspace.Open(req.archive, req.gameHint);
    workspace.Events().Pump();

    if (id == 0) {
        out.error = "no module matched " + req.archive.string() +
                    " -- pass --game gow2|gowr, or check the extension";
        return false;
    }

    Onyx::Modules::Document* doc = workspace.Get(id);
    if (!doc) {
        out.error = "internal error: Workspace::Open returned an id with no document";
        return false;
    }
    if (!doc->file || !doc->file->IsValid()) {
        out.error = "cannot open file: " + req.archive.string();
        return false;
    }

    out.vfs  = doc->mountedVfs;
    out.file = doc->file;

    // Bridge onto the legacy AssetContainer BuildSceneData (below) and every
    // downstream ITypeHandler still expect -- Task 4's scope is moving
    // container OPEN onto Workspace, not retiring AssetContainer itself
    // (Onyx::Domain::Wad.h documents it surviving for exactly this reason).
    // This bridge is exact for a flat WAD: doc->fileTable has one slot (the
    // container itself), same as AssetContainer::fileSource always assumed.
    // It is NOT exact for a mounted ISO: Gow2Module::ParseIsoToc pushes one
    // fileTable slot per PART*.PAK an entry actually lives in
    // (AssetEntry::source.fileIndex names which), but AssetContainer has
    // only ONE fileSource -- an entry whose fileIndex != 0 will read the
    // wrong bytes through this bridge. That is a real, structural limit of
    // AssetContainer's single-file model, not something Task 4 can close
    // without redesigning it (Phase 4/5 territory, same place WadBrowser's
    // own Onyx::Api::Database()/GetSelected() gap already lives) -- named
    // here rather than silently mis-decoding. See task-4-report.md.
    out.container.filename   = req.archive.filename().string();
    out.container.fullPath   = req.archive.string();
    out.container.fileSource = doc->fileTable.empty() ? doc->file : doc->fileTable[0];
    out.container.entries    = doc->roots;

    if (!parsedOk) {
        out.error = "ParseContainer failed (module: " + doc->module->Info().id + ")";
        return false;
    }

    out.ok = true;
    return true;
}

bool Load(const LoadRequest& req, LoadResult& out)
{
    if (!LoadContainer(req, out)) return false;
    out.ok = false;

    if (req.entry.empty()) {
        out.error = "no entry name given";
        return false;
    }

    out.entry = FindEntryByName(out.container.entries, req.entry);
    if (!out.entry) {
        out.error = "entry '" + req.entry + "' not found in " + out.container.filename;
        return false;
    }

    auto* handler = Onyx::Types::TypeRegistry::Get().Resolve(out.entry->typeId);
    if (!handler) {
        out.error = "no handler for type '" +
                    std::string(Onyx::Types::TypeCatalog::Get().Label(out.entry->typeId)) +
                    "' on entry '" + out.entry->name + "'";
        return false;
    }

    out.scene = handler->BuildSceneData(*out.entry, out.container);
    if (!out.scene) {
        out.error = "BuildSceneData returned null for '" + out.entry->name +
                    "' (handler: " + handler->GetName() + ")";
        return false;
    }

    out.ok = true;
    return true;
}

void PrintSceneStats(const Parsers::SceneData& scene, std::ostream& os)
{
    os << "\n=== Scene Data ===\n";
    os << "  Mesh parts  : " << scene.meshParts.size() << "\n";
    os << "  Materials   : " << scene.materials.size() << "\n";
    os << "  Textures    : " << scene.textures.size() << "\n";
    os << "  Has skeleton: " << (scene.HasSkeleton() ? "yes" : "no") << "\n";
    os << "  Is sky      : " << (scene.isSky ? "yes" : "no") << "\n";
    os << "  PBR layers  : " << (scene.pbrLayers ? "yes" : "no") << "\n";

    if (scene.skeleton)
        os << "  Joints      : " << scene.skeleton->joints.size() << "\n";

    size_t totalVerts = 0, totalIdx = 0;
    for (size_t i = 0; i < scene.meshParts.size(); ++i) {
        const auto& p = scene.meshParts[i];
        totalVerts += p.vertices.size();
        totalIdx   += p.indices.size();
        os << "  Part[" << i << "] '" << p.name << "'"
           << " matId=" << p.materialId
           << " layer=" << p.textureLayer
           << " verts=" << p.vertices.size()
           << " tris=" << p.indices.size() / 3
           << " joints=" << p.jointMap.size()
           << (p.isRigid ? " rigid" : "")
           << "\n";
    }
    os << "  TOTAL: " << totalVerts << " verts, " << totalIdx / 3 << " tris\n";

    for (size_t i = 0; i < scene.materials.size(); ++i) {
        const auto& mat = scene.materials[i];
        os << "  Mat[" << i << "] layers=" << mat.layers.size() << "\n";
        for (size_t j = 0; j < mat.layers.size(); ++j) {
            const auto& l = mat.layers[j];
            os << "    Layer[" << j << "] tex='" << l.textureName
               << "' hasTexture=" << (l.hasTexture ? 1 : 0) << "\n";
        }
    }

    for (size_t i = 0; i < scene.textures.size(); ++i) {
        for (size_t j = 0; j < scene.textures[i].size(); ++j) {
            const auto& td = scene.textures[i][j];
            if (td)
                os << "  Tex[" << i << "][" << j << "] " << td->width << "x" << td->height
                   << (td->IsValid() ? " OK" : " INVALID") << "\n";
            else
                os << "  Tex[" << i << "][" << j << "] null\n";
        }
    }
}

} // namespace Onyx::Harness
