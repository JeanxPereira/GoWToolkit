// Plain-text file handler. Routes .txt / .ini / .cfg / .csv / .json / .log
// PAK entries (typed by ProfileGOW2's extension switch) to the
// TextEditorViewer. Registered by TypeId — no magic number, so this never
// enters the magic dispatch map.

#include "core/domain/Entry.h"
#include "core/domain/Wad.h"
#include "core/types/ITypeHandler.h"
#include "core/types/TypeRegistry.h"
#include "core/vfs/IFile.h"
#include "fonts/SFSymbols.h"
#include "ui/viewers/TextEditorViewer.h"

namespace {

class TextPlainHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId GetId() const override { return GOW::TypeId::TextPlain; }
    const char* GetName() const override { return "Text"; }
    uint32_t    GetMagic() const override { return 0; } // extension-based
    const char* GetIcon() const override { return ICON_SF_DOCUMENT; }
    Color4f     GetColor() const override { return {0.85f, 0.85f, 0.85f, 1.0f}; }

    std::shared_ptr<GOW::IDocumentContent>
    CreateViewer(const ParsedEntry& entry, OpenWad& wad) override {
        if (!wad.fileSource) return nullptr;
        auto bytes = wad.fileSource->ReadAll();
        std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        return std::make_shared<GOW::TextEditorViewer>(entry.name, std::move(text));
    }
};

} // anonymous namespace

REGISTER_FILE_TYPE(TextPlainHandler);
