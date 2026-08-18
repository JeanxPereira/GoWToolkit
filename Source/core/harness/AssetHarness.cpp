#include "core/harness/AssetHarness.h"

#include <Onyx/Domain/IAssetProfile.h>
#include <Onyx/Services/ProfileManager.h>
#include <Onyx/Types/ITypeHandler.h>
#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Vfs/OsFile.h>

#include <algorithm>
#include <ostream>

namespace Onyx::Harness {

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

    // Profile selection mirrors AssetDatabase::LoadWad — an explicit hint wins,
    // and auto-detection is the fallback rather than a mutually exclusive
    // branch, so a wrong hint degrades to detection instead of hard-failing.
    auto& profiles = Onyx::Services::ProfileManager::Get();
    std::shared_ptr<Onyx::Domain::IAssetProfile> profile;
    if (!req.gameHint.empty())
        profile = profiles.FindProfileByHint(req.gameHint);
    if (!profile)
        profile = profiles.DetectProfileForFile(req.archive);
    if (!profile) {
        out.error = "no profile matched. Pass --game gow2|gowr";
        return false;
    }

    // An ISO needs the PAK-slice walk AssetDatabase::LoadPakFromIso does; the
    // harness deliberately does not reimplement it. Say so instead of failing
    // deeper down with a confusing parse error.
    auto ext = req.archive.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    if (ext == ".iso") {
        out.error = "ISO input is not supported headlessly yet. Run "
                    "`GoWTool extract <iso> <dir>` first, then point at a WAD.";
        return false;
    }

    // The GUI calls this before every ParseContainer. For GOWR it materialises
    // config.ini and drops a LOD index cached without it; skipping it is how
    // the CLI used to parse a different mesh set than the app.
    profile->PrepareForParse(req.archive);

    auto file = std::make_shared<Vfs::OsFile>(req.archive.string());
    if (!file->IsValid()) {
        out.error = "cannot open file: " + req.archive.string();
        return false;
    }
    out.file = file;

    out.container.filename   = req.archive.filename().string();
    out.container.fullPath   = req.archive.string();
    out.container.profile    = profile;
    out.container.fileSource = file;

    if (!profile->ParseContainer(file, out.container)) {
        out.error = "ParseContainer failed (profile: " + profile->GetName() + ")";
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
