#pragma once
#include "IDocumentContent.h"
#include <TextEditor.h>
#include <string>

namespace Onyx {

// Read-only text viewer backed by the Goossens fork of ImGuiColorTextEdit.
// Used for .txt / .ini / .cfg / .csv / .json / .log entries; future Lua / .scp
// support is a matter of registering a different LanguageDefinition.
class TextEditorViewer : public IDocumentContent {
public:
    TextEditorViewer(std::string name, std::string text);

    std::string GetName() const override { return m_name; }
    void Draw() override;

private:
    void PickLanguageByExtension();
    void DrawToolbar();
    void ExportAs();
    void CopyAll();

    // Builds a palette from the current ThemeManager accent + ImGui style
    // colors. Only chrome slots (background, cursor, selection, line numbers,
    // whitespace, matching brackets) are overridden; syntax-token colors stay
    // on the upstream Dark palette.
    static TextEditor::Palette BuildChromePalette();

    std::string m_name;
    std::string m_text;          // kept for ExportAs / CopyAll (byte-identical to source)
    TextEditor  m_editor;
    bool        m_showWhitespace = false;
    bool        m_showLineNumbers = true;
};

} // namespace Onyx
