#include "HexViewer.h"
#include <imgui.h>
#include <cstdio>

namespace MinimalViewer {

std::string FormatHexDump(const std::vector<uint8_t>& bytes, size_t maxBytes) {
    const size_t n = bytes.size() < maxBytes ? bytes.size() : maxBytes;
    std::string out;
    char line[128];
    for (size_t row = 0; row < n; row += 16) {
        std::snprintf(line, sizeof(line), "%08zX  ", row);
        out += line;
        for (size_t col = 0; col < 16; ++col) {
            if (row + col < n) { std::snprintf(line, sizeof(line), "%02X ", bytes[row + col]); out += line; }
            else               { out += "   "; }
        }
        out += " ";
        for (size_t col = 0; col < 16 && row + col < n; ++col) {
            uint8_t b = bytes[row + col];
            out += (b >= 32 && b < 127) ? char(b) : '.';
        }
        out += "\n";
    }
    return out;
}

HexViewer::HexViewer(std::string name, std::vector<uint8_t> bytes)
    : m_name(std::move(name)), m_bytes(std::move(bytes)) {
    m_dump = FormatHexDump(m_bytes, m_bytes.size());
}

void HexViewer::Draw() {
    ImGui::TextUnformatted("Raw bytes (hex):");
    ImGui::Separator();
    ImGui::BeginChild("hexdump", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushFont(nullptr); // default font; monospace not required for the smoke app
    ImGui::TextUnformatted(m_dump.c_str());
    ImGui::PopFont();
    ImGui::EndChild();
}

}
