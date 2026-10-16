#pragma once
#include <Onyx/Types/ITypeHandler.h>
#include <Onyx/Types/TypeCatalog.h>
#include "core/types/GameTypes.h"
#include <Onyx/Parsers/SceneNode.h>
#include <filesystem>
#include <memory>
#include <vector>

namespace Onyx {

// Every handler below resolves its TypeId through GowrModule's OWN catalog
// key, not through an Onyx::GameTypes::* extern.
//
// A GOWR tree is stamped entirely by GowrModule's RoleToTypeIdFn, so every
// node carries a module-minted id ("gowr.meshDefn", "gowr.texturePair", ...).
// The legacy externs are different ids under different keys, so handlers that
// returned them matched NOTHING a GOWR document contains -- the ViewerRegistry
// answered "No viewer found for TypeId=63" and not one GOWR asset would open,
// while the tree itself rendered perfectly. (GOW2 escaped this because
// ParseWadTagStream stamps a handler's legacy id whenever ResolveByTag finds
// one; GOWR has no such path.)
//
// Looked up per call rather than cached: GowrModule::RegisterTypes() runs when
// the module is added to a Workspace, which is later than these handlers'
// static construction, and a "not found" cached from too early would stick
// forever. TypeCatalog::Find is a hash lookup, and GetId() is called when the
// registry builds its index, not per frame.
inline Types::TypeId GowrType(const char* key) {
    return Types::TypeCatalog::Get().Find(key);
}

// Both MESH_* and MG_* are role MeshDefn (GowrTaxonomy.h), so the type cannot
// tell a rigged mesh from a plain one -- the NAME can, and always could. This
// used to be two handler classes returning the same TypeId, which the registry
// resolves by keeping the first and discarding the second: "GOWR Skinned Mesh"
// was unreachable, so nothing ever attached a rig. There is no SkinnedMesh
// role in the taxonomy for it to have claimed instead.
class GOWRMeshDefnHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GowrType("gowr.meshDefn"); }
    const char*  GetName()  const override { return "GOWR Mesh"; }
    std::shared_ptr<Schema::AssetNode> Parse(std::shared_ptr<Vfs::IFile> file) override;
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
    std::unique_ptr<Parsers::SceneData> BuildSceneData(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

// MG_*_gpu -- the mesh group's GPU buffer. Rigged by construction: a mesh
// group exists to carry the bone palette its parts index into.
class GOWRMeshGpuHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GowrType("gowr.meshGpu"); }
    const char*  GetName()  const override { return "GOWR Mesh GPU"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
    std::unique_ptr<Parsers::SceneData> BuildSceneData(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

class GOWRModelInstanceHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GowrType("gowr.gameObjectInst"); }
    const char*  GetName()  const override { return "GOWR Model Instance"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
    std::unique_ptr<Parsers::SceneData> BuildSceneData(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

class GOWRTextureHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GowrType("gowr.texturePair"); }
    const char*  GetName()  const override { return "GOWR Texture Pair"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};


class GOWRRigHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GowrType("gowr.gameObjectProto"); }
    const char*  GetName()  const override { return "GOWR Proto Rig"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
    std::unique_ptr<Parsers::SceneData> BuildSceneData(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

// One instance per shader stage, each holding its module catalog key.
//
// A string literal rather than a TypeId: these are constructed during static
// initialisation, long before any TypeId exists to copy. An earlier version
// took the id by value in the constructor and captured an empty handle
// forever, which the registry announced seven times at every launch
// ("Handler 'GOWR Shader' has no TypeId (catalog not seeded?)").
class GOWRShaderHandler : public Types::ITypeHandler {
public:
    explicit GOWRShaderHandler(const char* stageKey) : m_stageKey(stageKey) {}
    Types::TypeId  GetId()    const override { return GowrType(m_stageKey); }
    const char*  GetName()  const override { return "GOWR Shader"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
private:
    const char* m_stageKey;
};

class GOWRMaterialHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()   const override { return GowrType("gowr.material"); }
    const char*  GetName() const override { return "GOWR Material"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

// Which part and detail level a MeshPart belongs to. Comes from the mesh
// group's own part table rather than being inferred, so a part with a shorter
// LOD chain clamps to its last level instead of disappearing.
struct PartLevel {
    int part  = -1;
    int level = -1;
};

// LOD metadata BuildGowrScene reports alongside the scene. Only the viewer
// path consumes it.
struct GowrSceneMeta {
    std::vector<PartLevel> partLevels;
    int                    maxLevels = 1;
};

// LOD selection for BuildGowrScene.
//
//   kAllLevels -- every level at once. This is what the loader always did, and
//       it is wrong to look at: a mesh group's levels are alternative
//       representations of the same surface, so drawing five of them stacks
//       five shells of geometry on top of each other.
//   0, 1, 2... -- that detail level only. A part whose LOD chain is shorter
//       than the requested level clamps to its own last level rather than
//       disappearing, which is why this cannot be a simple equality test.
//
// kFinest (0) is the default: the most detailed level is what someone opening
// a model wants to see.
inline constexpr int kAllLevels = -1;
inline constexpr int kFinest    = 0;

// Builds the render-ready scene for a GOWR mesh/rig entry. Null means nothing
// to show; an empty non-null scene means "parsed fine, no geometry".
//
// `outMeta.partLevels` is reported for the parts that SURVIVE the filter, so
// it stays index-aligned with scene->meshParts.
std::unique_ptr<Parsers::SceneData> BuildGowrScene(const Domain::AssetEntry& entry,
                                                   Domain::AssetContainer& wad,
                                                   bool attachSkeleton,
                                                   GowrSceneMeta& outMeta,
                                                   int lodLevel = kFinest);

class TexPackIndex;
TexPackIndex& GetTexIndex();

// Try to auto-detect the GOWR game root from a loaded WAD path and persist it
// to config.ini next to the executable. Walks up from `wadPath` looking for a
// dir that contains `exec/wad/pc_le/`. Returns true if config.ini was already
// present or was written successfully.
bool EnsureGowrConfigIni(const std::filesystem::path& wadPath);

// Invalidate any cached LOD/tex index that was built before the config was
// written. Called by ProfileGOWR::PrepareForParse when a fresh config.ini is
// created.
void InvalidateLodIndex();

} // namespace Onyx
