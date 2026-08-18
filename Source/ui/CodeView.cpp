#include "CodeView.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace Onyx::Ui {

namespace {

// Mid-tone hues, picked so they keep enough contrast against both a dark and a
// light editor ground rather than only reading on one of them.
constexpr ImVec4 kColText     {0.82f, 0.84f, 0.85f, 1.00f};
constexpr ImVec4 kColComment  {0.45f, 0.52f, 0.50f, 1.00f};
constexpr ImVec4 kColKeyword  {0.85f, 0.45f, 0.72f, 1.00f};
constexpr ImVec4 kColType     {0.36f, 0.68f, 0.90f, 1.00f};
constexpr ImVec4 kColLocal    {0.52f, 0.78f, 0.58f, 1.00f};
constexpr ImVec4 kColGlobal   {0.90f, 0.66f, 0.34f, 1.00f};
constexpr ImVec4 kColMetadata {0.60f, 0.55f, 0.86f, 1.00f};
constexpr ImVec4 kColNumber   {0.83f, 0.62f, 0.45f, 1.00f};
constexpr ImVec4 kColString   {0.76f, 0.72f, 0.44f, 1.00f};
constexpr ImVec4 kColGutter   {0.40f, 0.44f, 0.46f, 1.00f};

bool IsIdentChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
}

bool IsKeyword(std::string_view w) {
    static const char* kWords[] = {
        "define", "declare", "call", "ret", "br", "phi", "select", "switch",
        "label", "align", "nsw", "nuw", "fast", "inbounds", "to", "unnamed_addr",
        "constant", "internal", "external", "private", "attributes", "target",
        "datalayout", "triple", "source_filename", "type", "undef", "null",
        "true", "false", "zeroinitializer", "preds", "immarg", "readonly",
        "readnone", "nounwind", "argmemonly", "speculatable", "willreturn",
        // HLSL, so the same view can host source when we have it
        "cbuffer", "struct", "return", "if", "else", "for", "while", "register",
        "SV_Position", "SV_Target", "numthreads", "in", "out", "inout",
    };
    for (const char* k : kWords) if (w == k) return true;
    return false;
}

bool IsTypeWord(std::string_view w) {
    static const char* kTypes[] = {
        "void", "half", "float", "double", "bool",
        "i1", "i8", "i16", "i32", "i64",
        "float2", "float3", "float4", "int2", "int3", "int4", "uint",
        "uint2", "uint3", "uint4", "matrix", "float4x4", "float3x3",
        "Texture2D", "SamplerState", "ByteAddressBuffer", "StructuredBuffer",
        "RWByteAddressBuffer", "RWStructuredBuffer",
    };
    for (const char* t : kTypes) if (w == t) return true;
    // LLVM vector/array sugar such as <4 x float> already colours per token.
    return false;
}

// Case-insensitive substring search, so the filter box behaves like a reader
// expects rather than like strcmp.
bool ContainsNoCase(std::string_view hay, std::string_view needle) {
    if (needle.empty()) return true;
    if (needle.size() > hay.size()) return false;
    const auto up = [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    };
    for (size_t i = 0; i + needle.size() <= hay.size(); ++i) {
        size_t j = 0;
        while (j < needle.size() && up(hay[i + j]) == up(needle[j])) ++j;
        if (j == needle.size()) return true;
    }
    return false;
}

void Emit(std::string_view s, const ImVec4& col, bool& first) {
    if (s.empty()) return;
    if (!first) ImGui::SameLine(0.0f, 0.0f);
    first = false;
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(s.data(), s.data() + s.size());
    ImGui::PopStyleColor();
}

} // namespace

void CodeView::SetText(std::string text) {
    m_text = std::move(text);
    m_lines.clear();
    m_matches.clear();
    m_currentMatch = -1;
    m_searchDirty  = true;

    std::string_view all(m_text);
    size_t start = 0;
    while (start <= all.size()) {
        size_t nl = all.find('\n', start);
        if (nl == std::string_view::npos) {
            m_lines.push_back(all.substr(start));
            break;
        }
        size_t end = nl;
        if (end > start && all[end - 1] == '\r') --end;   // tolerate CRLF
        m_lines.push_back(all.substr(start, end - start));
        start = nl + 1;
    }
    // A trailing newline produces one empty tail line; drop it so the gutter
    // does not show a phantom last line.
    if (!m_lines.empty() && m_lines.back().empty()) m_lines.pop_back();
}

void CodeView::Clear() {
    m_text.clear();
    m_lines.clear();
    m_matches.clear();
    m_currentMatch = -1;
    m_search[0] = '\0';
}

void CodeView::DrawLine(std::string_view line) const {
    bool first = true;
    size_t i = 0;

    while (i < line.size()) {
        const char c = line[i];

        if (c == ';') {                                   // comment to EOL
            Emit(line.substr(i), kColComment, first);
            return;
        }
        if (c == '"') {                                   // string literal
            size_t j = i + 1;
            while (j < line.size() && line[j] != '"') {
                if (line[j] == '\\' && j + 1 < line.size()) ++j;
                ++j;
            }
            if (j < line.size()) ++j;
            Emit(line.substr(i, j - i), kColString, first);
            i = j;
            continue;
        }
        if (c == '%' || c == '@' || c == '!') {           // sigil identifier
            size_t j = i + 1;
            while (j < line.size() && IsIdentChar(line[j])) ++j;
            const ImVec4& col = (c == '%') ? kColLocal
                              : (c == '@') ? kColGlobal
                                           : kColMetadata;
            Emit(line.substr(i, j - i), col, first);
            i = j;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '-' && i + 1 < line.size() &&
             std::isdigit(static_cast<unsigned char>(line[i + 1])))) {
            size_t j = i + 1;
            while (j < line.size() &&
                   (IsIdentChar(line[j]) || line[j] == '+' || line[j] == '-')) {
                // Stop at a '-' that starts a new token rather than an exponent.
                if (line[j] == '-' && !(line[j - 1] == 'e' || line[j - 1] == 'E')) break;
                ++j;
            }
            Emit(line.substr(i, j - i), kColNumber, first);
            i = j;
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t j = i;
            while (j < line.size() && IsIdentChar(line[j])) ++j;
            std::string_view w = line.substr(i, j - i);
            const ImVec4& col = IsKeyword(w) ? kColKeyword
                              : IsTypeWord(w) ? kColType
                                              : kColText;
            Emit(w, col, first);
            i = j;
            continue;
        }

        // Punctuation and whitespace run: emit verbatim.
        size_t j = i;
        while (j < line.size() && !std::isalnum(static_cast<unsigned char>(line[j])) &&
               line[j] != '_' && line[j] != '%' && line[j] != '@' && line[j] != '!' &&
               line[j] != '"' && line[j] != ';') {
            ++j;
        }
        if (j == i) ++j;
        Emit(line.substr(i, j - i), kColText, first);
        i = j;
    }

    if (first) ImGui::TextUnformatted("");   // keep empty lines occupying a row
}

void CodeView::Draw(const char* id, ImVec2 size) {
    ImGui::PushID(id);

    // -- Toolbar ------------------------------------------------------------
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::InputTextWithHint("##search", "Search", m_search, sizeof(m_search)))
        m_searchDirty = true;

    if (m_searchDirty) {
        m_searchDirty = false;
        m_matches.clear();
        m_currentMatch = -1;
        if (m_search[0] != '\0') {
            for (size_t i = 0; i < m_lines.size(); ++i)
                if (ContainsNoCase(m_lines[i], m_search)) m_matches.push_back(i);
        }
    }

    const bool haveMatches = !m_matches.empty();
    ImGui::SameLine();
    ImGui::BeginDisabled(!haveMatches);
    if (ImGui::Button("Prev") && haveMatches) {
        m_currentMatch = (m_currentMatch <= 0) ? (int)m_matches.size() - 1
                                               : m_currentMatch - 1;
        m_scrollToLine = (int)m_matches[m_currentMatch];
    }
    ImGui::SameLine();
    if (ImGui::Button("Next") && haveMatches) {
        m_currentMatch = (m_currentMatch + 1) % (int)m_matches.size();
        m_scrollToLine = (int)m_matches[m_currentMatch];
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (m_search[0] == '\0') {
        ImGui::TextDisabled("%zu lines", m_lines.size());
    } else if (haveMatches) {
        ImGui::TextDisabled("%d/%zu matches",
                            m_currentMatch < 0 ? 0 : m_currentMatch + 1,
                            m_matches.size());
    } else {
        ImGui::TextDisabled("no matches");
    }

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
    if (ImGui::Button("Copy")) ImGui::SetClipboardText(m_text.c_str());

    // -- Body ---------------------------------------------------------------
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.0f));
    ImGui::BeginChild("##body", size, true, ImGuiWindowFlags_HorizontalScrollbar);

    // Gutter width from the widest line number, so it never jitters.
    char widest[16];
    std::snprintf(widest, sizeof(widest), "%zu", m_lines.empty() ? 1 : m_lines.size());
    const float gutter = ImGui::CalcTextSize(widest).x + 12.0f;

    ImGuiListClipper clipper;
    clipper.Begin((int)m_lines.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            const bool isMatch = haveMatches && m_currentMatch >= 0 &&
                                 m_matches[m_currentMatch] == (size_t)i;
            if (isMatch) {
                const ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    p, ImVec2(p.x + ImGui::GetWindowWidth(),
                              p.y + ImGui::GetTextLineHeight()),
                    ImGui::GetColorU32(ImVec4(0.30f, 0.42f, 0.55f, 0.45f)));
            }

            ImGui::PushStyleColor(ImGuiCol_Text, kColGutter);
            ImGui::Text("%*d", (int)std::strlen(widest), i + 1);
            ImGui::PopStyleColor();
            ImGui::SameLine(gutter);

            ImGui::PushID(i);
            DrawLine(m_lines[i]);
            ImGui::PopID();
        }
    }
    clipper.End();

    if (m_scrollToLine >= 0) {
        ImGui::SetScrollY(m_scrollToLine * ImGui::GetTextLineHeightWithSpacing() -
                          ImGui::GetWindowHeight() * 0.4f);
        m_scrollToLine = -1;
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopID();
}

} // namespace Onyx::Ui
