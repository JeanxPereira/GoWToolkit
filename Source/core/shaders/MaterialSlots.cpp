#include "core/shaders/MaterialSlots.h"

#include "core/shaders/DxilDisassembler.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace Onyx {

namespace {

// The disassembler prints the buffer as a commented block:
//
//   ; cbuffer ConstBuf__materialData
//   ; {
//   ;   struct ConstBuf__materialData
//   ;   {
//   ;       struct struct.MaterialData
//   ;       {
//   ;           uint layer_0__diffuse;                    ; Offset:   28
//   ...
//   ;       } resourceTables__materialData;               ; Offset:    0
//
// Only the innermost member lines are wanted. They are recognised by carrying
// a second "; Offset:" on the same line after a declaration ending in ';',
// which the struct wrappers also do -- so the wrappers are excluded by name
// rather than by shape (they are the two that start with '}').
constexpr const char* kBufferMarker = "cbuffer ConstBuf__materialData";

std::string Trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

bool ParseMemberLine(const std::string& line, MaterialSlot& out) {
    const size_t off = line.find("; Offset:");
    if (off == std::string::npos) return false;

    // "<type> <name>;" is everything before that, minus the leading ';'
    std::string decl = line.substr(0, off);
    const size_t semi = decl.rfind(';');
    if (semi == std::string::npos) return false;
    decl = Trim(decl.substr(0, semi));
    if (!decl.empty() && decl.front() == ';') decl = Trim(decl.substr(1));
    if (decl.empty() || decl.front() == '}' || decl.front() == '{') return false;

    // The name is the last whitespace-separated token; anything before it is
    // the type, which may itself be several words ("struct struct.Foo").
    const size_t sp = decl.find_last_of(" \t");
    if (sp == std::string::npos) return false;
    std::string name = Trim(decl.substr(sp + 1));
    if (name.empty()) return false;

    const long v = std::strtol(line.c_str() + off + 9, nullptr, 10);
    if (v < 0 || v > 0xFFFF) return false;

    out.name   = std::move(name);
    out.offset = static_cast<uint16_t>(v);
    return true;
}

} // namespace

bool ReadMaterialSlots(const uint8_t* dxbc, size_t size,
                       std::vector<MaterialSlot>& out, std::string& error)
{
    out.clear();
    error.clear();
    if (!dxbc || size < 32) { error = "no DXBC container"; return false; }

    std::string text;
    if (!DxilDisassembler::Disassemble(dxbc, size, text, error)) return false;

    const size_t start = text.find(kBufferMarker);
    if (start == std::string::npos) {
        error = "no ConstBuf__materialData in this shader";
        return false;
    }

    // Stop at the wrapper's own closing line, which names the buffer again --
    // everything past it belongs to the next resource.
    size_t pos = start;
    int depth = 0;
    bool started = false;
    while (pos < text.size()) {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos) eol = text.size();
        const std::string line = text.substr(pos, eol - pos);
        pos = eol + 1;

        for (char c : line) {
            if (c == '{') { depth++; started = true; }
            else if (c == '}') depth--;
        }
        if (started && depth <= 0) break;

        MaterialSlot slot;
        if (ParseMemberLine(line, slot)) out.push_back(std::move(slot));
    }

    if (out.empty()) {
        error = "ConstBuf__materialData declared no members";
        return false;
    }
    std::sort(out.begin(), out.end(),
              [](const MaterialSlot& a, const MaterialSlot& b) { return a.offset < b.offset; });
    return true;
}

bool DeclaresCoverageWithoutOcclusion(const std::vector<MaterialSlot>& slots) {
    bool coverage = false, occlusion = false;
    for (const auto& s : slots) {
        // Suffix match on the channel, so it holds for any layer index.
        if (s.name.size() > 7 &&
            (s.name.compare(s.name.size() - 7, 7, "__alpha") == 0)) coverage = true;
        if (s.name.find("__opacity") != std::string::npos) coverage = true;
        if (s.name.size() > 4 && s.name.compare(s.name.size() - 4, 4, "__ao") == 0)
            occlusion = true;
        if (s.name.find("__occlusion") != std::string::npos) occlusion = true;
    }
    return coverage && !occlusion;
}

} // namespace Onyx
