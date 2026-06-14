#pragma once
#include <Onyx/Viewers/IDocumentContent.h>
#include <string>
#include <vector>
#include <cstdint>

namespace MinimalViewer {

// Pure formatting helper (testable without ImGui): classic 16-byte-per-row
// hex dump with an offset column and an ASCII gutter, capped at maxBytes.
std::string FormatHexDump(const std::vector<uint8_t>& bytes, size_t maxBytes);

// An app-authored document viewer that renders raw bytes as hex. Proves the
// public Onyx::Viewers::IDocumentContent interface is implementable outside
// the engine.
class HexViewer : public Onyx::Viewers::IDocumentContent {
public:
    HexViewer(std::string name, std::vector<uint8_t> bytes);

    std::string GetName() const override { return m_name; }
    void Draw() override;

private:
    std::string m_name;
    std::vector<uint8_t> m_bytes;
    std::string m_dump;   // precomputed
};

}
