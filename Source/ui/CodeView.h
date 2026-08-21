#pragma once
#include <imgui.h>
#include <string>
#include <string_view>
#include <vector>

namespace Onyx::Ui {

// Read-only source viewer with line numbers, search and syntax colouring.
//
// Tuned for DXIL/LLVM-IR disassembly, which is what the shader viewer shows,
// but the tokeniser is generic enough for HLSL too: it colours line comments,
// identifiers by sigil (%local, @global, !metadata), keywords, types, numbers
// and strings.
//
// Only the visible lines are tokenised, so a multi-thousand-line disassembly
// costs the same as a short one.
class CodeView {
public:
    // Takes ownership of the text and indexes its lines. Safe to call again to
    // replace the contents.
    void SetText(std::string text);
    void Clear();

    bool   Empty() const { return m_lines.empty(); }
    size_t LineCount() const { return m_lines.size(); }
    const std::string& Text() const { return m_text; }

    // Draws the toolbar and the scrolling body. `size` follows the ImGui child
    // convention: zero means "fill the remaining space".
    void Draw(const char* id, ImVec2 size = ImVec2(0.0f, 0.0f));

private:
    void DrawLine(std::string_view line) const;

    std::string                   m_text;
    std::vector<std::string_view> m_lines;   // views into m_text
    std::vector<size_t>           m_matches; // line indices matching m_search
    char                          m_search[128] = {};
    bool                          m_searchDirty = false;
    int                           m_currentMatch = -1;
    int                           m_scrollToLine = -1;
};

} // namespace Onyx::Ui
