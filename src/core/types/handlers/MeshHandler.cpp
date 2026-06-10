// Mesh handler — raw GPU geometry data
// Magic: 0x0001000F (MESH_MAGIC in god_of_war_browser)
// Also handles GMDL_MAGIC = 0x0003000F

#include "core/types/TypeRegistry.h"
#include "core/types/ITypeHandler.h"
#include "fonts/SFSymbols.h"

namespace {

class MeshHandler : public Onyx::ITypeHandler {
public:
    Onyx::TypeId  GetId()    const override { return Onyx::TypeId::Mesh; }
    const char*  GetName()  const override { return "Mesh"; }
    uint32_t     GetMagic() const override { return 0x0001000F; }
    const char*  GetIcon()  const override { return ICON_SF_CUBE_FILL; }
    Color4f      GetColor() const override { return {0.4f, 0.8f, 1.0f, 1.0f}; }

    std::shared_ptr<Onyx::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        if (auto* handler = Onyx::TypeRegistry::Get().Resolve(Onyx::TypeId::Model)) {
            return handler->CreateViewer(entry, wad);
        }
        return nullptr;
    }
};

class GmdlHandler : public Onyx::ITypeHandler {
public:
    Onyx::TypeId  GetId()    const override { return Onyx::TypeId::Mesh; }
    const char*  GetName()  const override { return "GMDL Mesh"; }
    uint32_t     GetMagic() const override { return 0x0003000F; }
    const char*  GetIcon()  const override { return ICON_SF_CUBE_FILL; }
    Color4f      GetColor() const override { return {0.4f, 0.8f, 1.0f, 1.0f}; }

    std::shared_ptr<Onyx::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        if (auto* handler = Onyx::TypeRegistry::Get().Resolve(Onyx::TypeId::Model)) {
            return handler->CreateViewer(entry, wad);
        }
        return nullptr;
    }
};

} // anonymous namespace

REGISTER_TYPE(GOW2, MeshHandler);
REGISTER_TYPE(GOW2, GmdlHandler);
