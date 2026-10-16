#pragma once

// One open → parse → build-scene path, shared by every headless consumer.
//
// The GUI reaches `ITypeHandler::BuildSceneData` through AssetDatabase; the CLI
// used to reach it through a private copy of the same steps inside CliApp.cpp.
// Two copies means a headless screenshot can disagree with the viewport for
// reasons that have nothing to do with the asset — which would make the
// screenshot worthless as evidence. Everything non-GUI now goes through here.
//
// Onyx v1.1 deleted IAssetProfile/ProfileManager/AssetDatabase; opening a
// container now goes through Onyx::Modules::Workspace + IGameModule
// (Gow2Module/GowrModule, Phase 2 Tasks 2/3). LoadContainer (below) opens
// through a Workspace the same way the GUI's registrar does (App::AddModule,
// see AppRegistration.cpp) -- the GOWR config.ini/LOD-index ordering that
// used to be IAssetProfile::PrepareForParse's job now runs at the top of
// GowrModule::ParseContainer itself (see GowrModule.cpp), so there is no
// separate pre-parse step for this harness to call or skip. `out.container`
// is a bridge onto the legacy AssetContainer BuildSceneData below still
// expects (Onyx::Domain::Wad.h: "AssetContainer survives as the generic
// per-entry type ViewerRegistry/ITypeHandler still pass around for viewer
// dispatch") -- see LoadContainer's own comment for what that bridge does
// and does not cover.

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Onyx/Parsers/SceneNode.h>
#include <Onyx/Vfs/IFile.h>
#include <Onyx/Vfs/IVirtualFileSystem.h>

#include "core/WadTypes.h"

namespace Onyx::Harness {

struct LoadRequest {
    std::filesystem::path archive;   // a game container (GOWR .wad, GOW2 .iso/.wad)
    std::string           entry;     // entry name to build a scene from
    std::string           gameHint;  // "gow2" | "gowr" | "ragnarok" | "" to auto-detect
};

/// Owns everything the scene points into. `entry` is a pointer into
/// `container`, so this object must outlive any use of the scene.
struct LoadResult {
    bool        ok = false;
    std::string error;

    std::shared_ptr<Vfs::IVirtualFileSystem> vfs;   // non-null only for mounted archives
    std::shared_ptr<Vfs::IFile>              file;  // keeps the backing storage alive
    AssetContainer                           container;
    const AssetEntry*                        entry = nullptr;
    /// Which IGameModule claimed the file ("gowr", "gow2"). Reported so a
    /// caller can take a game-specific step without re-probing.
    std::string                              moduleId;
    std::unique_ptr<Parsers::SceneData>      scene;

    LoadResult() = default;
    LoadResult(const LoadResult&) = delete;
    LoadResult& operator=(const LoadResult&) = delete;
};

/// Opens `req.archive` and parses its entry tree. `req.entry` is ignored.
/// This is the half of `Load` that every command needs, including the ones
/// that only list entries.
bool LoadContainer(const LoadRequest& req, LoadResult& out);

/// Opens `req.archive`, parses it, locates `req.entry` and builds its scene.
/// Fills `out` and returns `out.ok`. On failure `out.error` says why, and any
/// stage that did succeed is still populated (so a caller can, for instance,
/// list the container's entries after a name lookup misses).
bool Load(const LoadRequest& req, LoadResult& out);

/// Depth-first exact-name lookup across an entry tree.
const AssetEntry* FindEntryByName(const std::vector<AssetEntry>& entries,
                                  std::string_view name);

/// Appends every entry name in the tree to `out` (used for "not found" help).
void CollectEntryNames(const std::vector<AssetEntry>& entries,
                       std::vector<std::string>& out);

/// Human-readable dump of a built scene: parts, materials, textures, totals.
void PrintSceneStats(const Parsers::SceneData& scene, std::ostream& os);

} // namespace Onyx::Harness
