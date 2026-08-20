// Mesh handler — raw GPU geometry data
// Magic: 0x0001000F (MESH_MAGIC in god_of_war_browser)
// Also handles GMDL_MAGIC = 0x0003000F

#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Types/ITypeHandler.h>
#include "core/types/WadDispatch.h"
#include "core/types/GameTypes.h"
#include <Onyx/Fonts/SFSymbols.h>

namespace {

class MeshHandler : public Onyx::Gow::IWadTypeHandler {
public:
    Onyx::Types::TypeId  GetId()    const override { return Onyx::GameTypes::Mesh; }
    const char*  GetName()  const override { return "Mesh"; }
    uint32_t     GetMagic() const override { return 0x0001000F; }
    const char*  GetIcon()  const override { return ICON_SF_CUBE_FILL; }
    Color4f      GetColor() const override { return {0.4f, 0.8f, 1.0f, 1.0f}; }

    std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        if (auto* handler = Onyx::Types::TypeRegistry::Get().Resolve(Onyx::GameTypes::Model)) {
            return handler->CreateViewer(entry, wad);
        }
        return nullptr;
    }
};

class GmdlHandler : public Onyx::Gow::IWadTypeHandler {
public:
    Onyx::Types::TypeId  GetId()    const override { return Onyx::GameTypes::Mesh; }
    const char*  GetName()  const override { return "GMDL Mesh"; }
    uint32_t     GetMagic() const override { return 0x0003000F; }
    const char*  GetIcon()  const override { return ICON_SF_CUBE_FILL; }
    Color4f      GetColor() const override { return {0.4f, 0.8f, 1.0f, 1.0f}; }

    std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        if (auto* handler = Onyx::Types::TypeRegistry::Get().Resolve(Onyx::GameTypes::Model)) {
            return handler->CreateViewer(entry, wad);
        }
        return nullptr;
    }
};

} // anonymous namespace

REGISTER_GOW_TYPE(GOW2, MeshHandler);
REGISTER_GOW_TYPE(GOW2, GmdlHandler);
