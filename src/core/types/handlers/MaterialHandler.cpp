// Material handler — GOW1/2 material definition
// Magic: 0x00000008 (MAT_MAGIC in god_of_war_browser)

#include "core/types/TypeRegistry.h"
#include "core/types/ITypeHandler.h"

#include "ui/viewers/MaterialViewer.h"
#include "core/parsers/gow2/MaterialParser.h"
#include "core/parsers/gow2/TextureParser.h"
#include "core/vfs/SliceFile.h"
#include "fonts/SFSymbols.h"

namespace {

static const ParsedEntry* FindEntryWithPayload(const std::vector<ParsedEntry>& entries, const std::string& name, GOW::TypeId type) {
    for (const auto& entry : entries) {
        if (entry.typeId == type && entry.name == name && entry.size > 0)
            return &entry;
        if (!entry.children.empty()) {
            if (auto found = FindEntryWithPayload(entry.children, name, type))
                return found;
        }
    }
    return nullptr;
}

static const ParsedEntry* FindTextureEntry(const std::vector<ParsedEntry>& entries, const std::string& name) {
    for (const auto& entry : entries) {
        if (entry.typeId == GOW::TypeId::Texture && entry.name == name)
            return &entry;
        if (!entry.children.empty()) {
            if (auto found = FindTextureEntry(entry.children, name))
                return found;
        }
    }
    return nullptr;
}

class MaterialHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::Material; }
    const char*  GetName()  const override { return "Material"; }
    uint32_t     GetMagic() const override { return 0x00000008; }
    const char*  GetIcon()  const override { return ICON_SF_PAINTPALETTE_FILL; }  // symbol-color
    Color4f      GetColor() const override { return {0.95f, 0.6f, 0.2f, 1.0f}; }  // orange

    std::shared_ptr<GOW::IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override {
        if (!wad.fileSource) return nullptr;
        
        const ParsedEntry* matEntryToParse = &entry;
        if (matEntryToParse->size == 0) {
            if (auto realMat = FindEntryWithPayload(wad.entries, matEntryToParse->name, GOW::TypeId::Material))
                matEntryToParse = realMat;
        }

        auto matData = GOW::GOW2MaterialParser::Parse(*matEntryToParse, wad.fileSource);

        if (matData) {
            // Resolve textures for each layer
            std::vector<std::unique_ptr<GOW::TextureData>> textures;
            for (const auto& layer : matData->layers) {
                std::unique_ptr<GOW::TextureData> texData = nullptr;
                if (layer.hasTexture && !layer.textureName.empty()) {
                    if (auto* texEntry = FindTextureEntry(wad.entries, layer.textureName)) {
                        texData = GOW::GOW2TextureParser::Parse(*texEntry, wad.entries, wad.fileSource);
                    }
                }
                textures.push_back(std::move(texData));
            }

            return std::make_shared<GOW::MaterialViewer>(
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

REGISTER_TYPE(GOW2, MaterialHandler);

