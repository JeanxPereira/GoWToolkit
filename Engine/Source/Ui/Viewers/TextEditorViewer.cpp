#include "TextEditorViewer.h"
#include "UIHelpers.h"
#include "Core/FontManager.h"
#include "Core/Logger.h"
#include "Core/ThemeManager.h"
#include "Fonts/SFSymbols.h"
#include "Ui/Widgets.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace Onyx::Viewers {

TextEditorViewer::TextEditorViewer(std::string name, std::string text)
    : m_name(std::move(name)), m_text(std::move(text)) {
    m_editor.SetText(m_text);
    m_editor.SetReadOnlyEnabled(true);
    m_editor.SetShowLineNumbersEnabled(m_showLineNumbers);
    m_editor.SetShowWhitespacesEnabled(m_showWhitespace);
    m_editor.SetTabSize(4);
    PickLanguageByExtension();
}

void TextEditorViewer::PickLanguageByExtension() {
    auto dot = m_name.find_last_of('.');
    if (dot == std::string::npos) return;
    std::string ext = m_name.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });

    // Library only ships highlighters for code-style languages. For
    // .ini / .cfg / .csv / .txt / .log we leave the editor with no language
    // set (plain monochrome text).
    if (ext == "json") {
        m_editor.SetLanguage(TextEditor::Language::Json());
    } else if (ext == "lua") {
        m_editor.SetLanguage(TextEditor::Language::Lua());
    } else if (ext == "md" || ext == "markdown") {
        m_editor.SetLanguage(TextEditor::Language::Markdown());
    }
}

void TextEditorViewer::ExportAs() {
    std::string path = SystemSaveFileDialog(m_name);
    if (path.empty()) return;
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        LOG_ERR("[TextEditorViewer] Cannot open %s for write", path.c_str());
        return;
    }
    out.write(m_text.data(), (std::streamsize)m_text.size());
    LOG_INFO("[TextEditorViewer] Exported %s (%zu bytes)", path.c_str(), m_text.size());
}

void TextEditorViewer::CopyAll() {
    ImGui::SetClipboardText(m_text.c_str());
}

void TextEditorViewer::DrawToolbar() {
    namespace W = Onyx::UI::Widgets;

    ImGui::PushStyleColor(ImGuiCol_Button,        Onyx::Theme::ToolbarButton());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Onyx::Theme::ToolbarButtonHover());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Onyx::Theme::ToolbarButtonActive());

    {
        W::IconButtonOpts opts;
        opts.tooltip = "Export Asâ€¦";
        if (W::IconButton("txt_export", ICON_SF_SQUARE_AND_ARROW_DOWN, opts)) {
            ExportAs();
        }
    }
    ImGui::SameLine();
    {
        W::IconButtonOpts opts;
        opts.tooltip = "Copy All";
        if (W::IconButton("txt_copy", ICON_SF_DOCUMENT_ON_DOCUMENT, opts)) {
            CopyAll();
        }
    }
    ImGui::SameLine();
    {
        W::IconButtonOpts opts;
        opts.tooltip  = "Line numbers";
        opts.selected = m_showLineNumbers;
        if (W::IconButton("txt_lines", ICON_SF_LIST_BULLET, opts)) {
            m_showLineNumbers = !m_showLineNumbers;
            m_editor.SetShowLineNumbersEnabled(m_showLineNumbers);
        }
    }
    ImGui::SameLine();
    {
        W::IconButtonOpts opts;
        opts.tooltip  = "Show whitespace";
        opts.selected = m_showWhitespace;
        if (W::IconButton("txt_ws", ICON_SF_TEXTFORMAT, opts)) {
            m_showWhitespace = !m_showWhitespace;
            m_editor.SetShowWhitespacesEnabled(m_showWhitespace);
        }
    }

    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::TextDisabled("| %zu bytes", m_text.size());
}

TextEditor::Palette TextEditorViewer::BuildChromePalette() {
    using C = TextEditor::Color;
    TextEditor::Palette p = TextEditor::GetDarkPalette(); // keep syntax tokens

    const ImVec4 accent   = Onyx::Theme::GetAccent();
    const ImGuiStyle& st  = ImGui::GetStyle();
    const ImVec4 windowBg = st.Colors[ImGuiCol_WindowBg];
    const ImVec4 textDim  = st.Colors[ImGuiCol_TextDisabled];

    auto u32    = [](ImVec4 c)            { return ImGui::ColorConvertFloat4ToU32(c); };
    auto opaque = [](ImVec4 c)            { c.w = 1.0f; return c; };
    auto alpha  = [](ImVec4 c, float a)   { c.w = a;    return c; };

    p[(size_t)C::background]                = u32(opaque(windowBg));
    p[(size_t)C::cursor]                    = u32(opaque(accent));
    p[(size_t)C::selection]                 = u32(alpha(accent, 0.35f));
    p[(size_t)C::lineNumber]                = u32(textDim);
    p[(size_t)C::currentLineNumber]         = u32(opaque(accent));
    p[(size_t)C::whitespace]                = u32(alpha(textDim, 0.50f));
    p[(size_t)C::matchingBracketBackground] = u32(alpha(accent, 0.20f));
    p[(size_t)C::matchingBracketActive]     = u32(alpha(accent, 0.50f));
    // matchingBracketLevel1/2/3, matchingBracketError, and all syntax-token
    // slots (keyword/string/number/comment/â€¦) inherit from GetDarkPalette().
    return p;
}

void TextEditorViewer::Draw() {
    // Re-sync palette every frame so the editor chrome animates along with
    // ThemeManager transitions (~250 ms) without needing to close + reopen
    // the tab when the accent changes.
    m_editor.SetPalette(BuildChromePalette());

    DrawToolbar();
    ImGui::Separator();

    ImFont* mono = Onyx::Fonts::GetMonoFont();
    if (mono) ImGui::PushFont(mono);
    m_editor.Render("##editor", ImVec2(0, 0), false);
    if (mono) ImGui::PopFont();
}

} // namespace Onyx::Viewers
