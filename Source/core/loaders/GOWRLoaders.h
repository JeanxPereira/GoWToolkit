#pragma once
#include <Onyx/Types/ITypeHandler.h>
#include "core/types/GameTypes.h"
#include <filesystem>

namespace Onyx {

class GOWRMeshDefnHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GameTypes::MeshDefn; }
    const char*  GetName()  const override { return "GOWR Mesh Defn"; }
    std::shared_ptr<Schema::AssetNode> Parse(std::shared_ptr<Vfs::IFile> file) override;
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

class GOWRSkinnedMeshHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GameTypes::MeshDefn; } // reuse
    const char*  GetName()  const override { return "GOWR Skinned Mesh"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

class GOWRModelInstanceHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GameTypes::GameObjectInst; }
    const char*  GetName()  const override { return "GOWR Model Instance"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

class GOWRTextureHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GameTypes::TexturePair; }
    const char*  GetName()  const override { return "GOWR Texture Pair"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};


class GOWRRigHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()    const override { return GameTypes::GameObjectProto; }
    const char*  GetName()  const override { return "GOWR Proto Rig"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

// One instance per shader stage.
//
// The stage is stored as a POINTER to the GameTypes extern, not as a copy of
// its value. These handlers are constructed during static initialisation, and
// GameTypes::Shader* are only filled in by RegisterGameTypes() from main() --
// so a constructor that copied the value captured an invalid handle forever,
// and the registry reported "Handler 'GOWR Shader' has no TypeId (catalog not
// seeded?)" seven times at every startup, leaving all seven shader types
// without a viewer. Reading through the pointer in GetId() resolves it at call
// time, after the catalog is seeded, which is what every other handler in this
// tree does implicitly by naming the extern inside GetId().
class GOWRShaderHandler : public Types::ITypeHandler {
public:
    explicit GOWRShaderHandler(const Types::TypeId* stage) : m_stage(stage) {}
    Types::TypeId  GetId()    const override { return m_stage ? *m_stage : Types::TypeId{}; }
    const char*  GetName()  const override { return "GOWR Shader"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
private:
    const Types::TypeId* m_stage;
};

class GOWRMaterialHandler : public Types::ITypeHandler {
public:
    Types::TypeId  GetId()   const override { return GameTypes::GowrMaterial; }
    const char*  GetName() const override { return "GOWR Material"; }
    std::shared_ptr<Viewers::IDocumentContent> CreateViewer(const Domain::AssetEntry& entry, Domain::AssetContainer& wad) override;
};

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
