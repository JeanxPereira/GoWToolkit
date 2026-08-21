// Material handler — GOW1/2 material definition
// Magic: 0x00000008 (MAT_MAGIC in god_of_war_browser)

#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Types/ITypeHandler.h>
#include "core/types/WadDispatch.h"
#include "core/types/GameTypes.h"

#include "ui/viewers/MaterialViewer.h"
#include "core/parsers/gow2/MaterialParser.h"
#include "core/parsers/gow2/TextureParser.h"
#include <Onyx/Vfs/SliceFile.h>
#include <Onyx/Fonts/SFSymbols.h>

namespace {

static const AssetEntry* FindEntryWithPayload(const std::vector<AssetEntry>& entries, const std::string& name, Onyx::Types::TypeId type) {
    for (const auto& entry : entries) {
        if (entry.typeId == type && entry.name == name && entry.source.size > 0)
            return &entry;
        if (!entry.children.empty()) {
            if (auto found = FindEntryWithPayload(entry.children, name, type))
                return found;
        }
    }
    return nullptr;
}

static const AssetEntry* FindTextureEntry(const std::vector<AssetEntry>& entries, const std::string& name) {
    for (const auto& entry : entries) {
        if (entry.typeId == Onyx::GameTypes::Texture && entry.name == name)
            return &entry;
        if (!entry.children.empty()) {
            if (auto found = FindTextureEntry(entry.children, name))
                return found;
        }
    }
    return nullptr;
}

class MaterialHandler : public Onyx::Gow::IWadTypeHandler {
public:
    Onyx::Types::TypeId  GetId()    const override { return Onyx::GameTypes::Material; }
    const char*  GetName()  const override { return "Material"; }
    uint32_t     GetMagic() const override { return 0x00000008; }
    const char*  GetIcon()  const override { return ICON_SF_PAINTPALETTE_FILL; }  // symbol-color
    Color4f      GetColor() const override { return {0.95f, 0.6f, 0.2f, 1.0f}; }  // orange

    std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        if (!wad.fileSource) return nullptr;
        
        const AssetEntry* matEntryToParse = &entry;
        if (matEntryToParse->source.size == 0) {
            if (auto realMat = FindEntryWithPayload(wad.entries, matEntryToParse->name, Onyx::GameTypes::Material))
                matEntryToParse = realMat;
        }

        auto matData = Onyx::GOW2MaterialParser::Parse(*matEntryToParse, wad.fileSource);

        if (matData) {
            // Resolve textures for each layer
            std::vector<std::unique_ptr<Onyx::Parsers::TextureData>> textures;
            for (const auto& layer : matData->layers) {
                std::unique_ptr<Onyx::Parsers::TextureData> texData = nullptr;
                if (layer.hasTexture && !layer.textureName.empty()) {
                    if (auto* texEntry = FindTextureEntry(wad.entries, layer.textureName)) {
                        texData = Onyx::GOW2TextureParser::Parse(*texEntry, wad.entries, wad.fileSource);
                    }
                }
                textures.push_back(std::move(texData));
            }

            return std::make_shared<Onyx::MaterialViewer>(
                entry.name, 
                std::move(matData), 
                [](const std::string& texName) -> unsigned int {
                    return 0; // Fallback — textures are now passed directly
                },
                std::move(textures)
            );
        }
        return nullptr;
    }
};

} // anonymous namespace

REGISTER_GOW_TYPE(GOW2, MaterialHandler);


