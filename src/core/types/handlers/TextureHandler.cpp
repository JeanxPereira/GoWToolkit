// Texture handler — texture reference node
// Magic: 0x00000007 (TXR_MAGIC in god_of_war_browser)

#include "core/types/TypeRegistry.h"
#include "core/types/ITypeHandler.h"

#include "ui/viewers/ImageViewer.h"
#include "core/parsers/gow2/TextureParser.h"
#include "fonts/SFSymbols.h"

namespace {

class TextureHandler : public Onyx::ITypeHandler {
public:
    Onyx::TypeId  GetId()    const override { return Onyx::GameTypes::Texture; }
    const char*  GetName()  const override { return "Texture"; }
    uint32_t     GetMagic() const override { return 0x00000007; }
    const char*  GetIcon()  const override { return ICON_SF_PHOTO; }  // file-media
    Color4f      GetColor() const override { return {1.0f, 0.5f, 0.8f, 1.0f}; }  // rosa

    std::shared_ptr<Onyx::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        if (!wad.fileSource) return nullptr;
        auto texData = Onyx::GOW2TextureParser::Parse(entry, wad.entries, wad.fileSource);
        if (texData && texData->IsValid())
            return std::make_shared<Onyx::ImageViewer>(entry.name, std::move(texData));
        return nullptr;
    }
};

} // anonymous namespace

REGISTER_TYPE(GOW2, TextureHandler);
