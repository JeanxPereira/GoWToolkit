#pragma once
#include "core/types/ITypeHandler.h"

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

} // namespace GOW
