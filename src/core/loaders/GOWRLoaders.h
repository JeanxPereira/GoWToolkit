#pragma once
#include "core/types/ITypeHandler.h"
#include <filesystem>

namespace GOW {

class GOWRMeshDefnHandler : public ITypeHandler {
public:
    TypeId  GetId()    const override { return TypeId::MeshDefn; }
    const char*  GetName()  const override { return "GOWR Mesh Defn"; }
    uint32_t     GetMagic() const override { return 0x00; }
    std::shared_ptr<AssetNode> Parse(std::shared_ptr<IFile> file) override;
    std::shared_ptr<IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOWRSkinnedMeshHandler : public ITypeHandler {
public:
    TypeId  GetId()    const override { return TypeId::MeshDefn; } // reuse
    const char*  GetName()  const override { return "GOWR Skinned Mesh"; }
    uint32_t     GetMagic() const override { return 0x00; }
    std::shared_ptr<IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOWRModelInstanceHandler : public ITypeHandler {
public:
    TypeId  GetId()    const override { return TypeId::GameObjectInst; }
    const char*  GetName()  const override { return "GOWR Model Instance"; }
    uint32_t     GetMagic() const override { return 0x00; }
    std::shared_ptr<IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOWRTextureHandler : public ITypeHandler {
public:
    TypeId  GetId()    const override { return TypeId::TexturePair; }
    const char*  GetName()  const override { return "GOWR Texture Pair"; }
    uint32_t     GetMagic() const override { return 0x00; }
    std::shared_ptr<IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override;
};


class GOWRRigHandler : public ITypeHandler {
public:
    TypeId  GetId()    const override { return TypeId::GameObjectProto; }
    const char*  GetName()  const override { return "GOWR Proto Rig"; }
    uint32_t     GetMagic() const override { return 0x00; }
    std::shared_ptr<IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOWRShaderHandler : public ITypeHandler {
public:
    GOWRShaderHandler(TypeId id) : m_id(id) {}
    TypeId  GetId()    const override { return m_id; }
    const char*  GetName()  const override { return "GOWR Shader"; }
    uint32_t     GetMagic() const override { return 0x00; }
    std::shared_ptr<IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override;
private:
    TypeId m_id;
};

class TexPackIndex;
TexPackIndex& GetTexIndex();

// Try to auto-detect the GOWR game root from a loaded WAD path and persist it
// to config.ini next to the executable. Walks up from `wadPath` looking for a
// dir that contains `exec/wad/pc_le/`. Returns true if config.ini was already
// present or was written successfully.
bool EnsureGowrConfigIni(const std::filesystem::path& wadPath);

} // namespace GOW
