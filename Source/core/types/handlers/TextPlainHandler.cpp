// Plain-text file handler. Routes .txt / .ini / .cfg / .csv / .json / .log
// PAK entries (typed by ProfileGOW2's extension switch) to the
// Viewers::TextEditorViewer. Registered by TypeId Ã¢â‚¬â€ no magic number, so this never
// enters the magic dispatch map.

#include <Onyx/Domain/Entry.h>
#include <Onyx/Domain/Wad.h>
#include <Onyx/Types/ITypeHandler.h>
#include <Onyx/Types/TypeRegistry.h>
#include "core/types/GameTypes.h"
#include <Onyx/Vfs/IFile.h>
#include <Onyx/Fonts/SFSymbols.h>
#include <Onyx/Viewers/TextEditorViewer.h>

namespace {

class TextPlainHandler : public Onyx::Types::ITypeHandler {
public:
    Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::TextPlain; }
    const char* GetName() const override { return "Text"; }
    const char* GetIcon() const override { return ICON_SF_DOCUMENT; }
    Color4f     GetColor() const override { return {0.85f, 0.85f, 0.85f, 1.0f}; }

    std::shared_ptr<Onyx::Viewers::IDocumentContent>
    CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        if (!wad.fileSource) return nullptr;
        auto bytes = wad.fileSource->ReadAll();
        std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        return std::make_shared<Onyx::Viewers::TextEditorViewer>(entry.name, std::move(text));
    }
};

} // anonymous namespace

REGISTER_FILE_TYPE(TextPlainHandler);
