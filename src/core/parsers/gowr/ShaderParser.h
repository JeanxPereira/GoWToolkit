#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "core/vfs/IFile.h"

namespace GOW {

// ── DXBC Chunk descriptor ──────────────────────────────────────────────────
struct DxbcChunk {
    char     tag[5] = {};     // FourCC + null terminator (e.g. "ISG1", "DXIL")
    uint32_t offset = 0;      // offset within DXBC payload
    uint32_t size   = 0;      // chunk data size (excludes 8-byte header)
};

// ── Signature element (parsed from ISG1/OSG1/PSG1) ─────────────────────────
struct SignatureElement {
    std::string semanticName;
    uint32_t    semanticIndex    = 0;
    uint32_t    systemValueType  = 0;   // 0=none, 1=SV_Position, etc.
    uint32_t    componentType    = 0;   // 0=unknown, 1=uint, 2=int, 3=float
    uint32_t    registerIndex    = 0;
    uint8_t     mask             = 0;   // XYZW bit mask
    uint8_t     readWriteMask    = 0;
};

// ── STAT chunk summary ─────────────────────────────────────────────────────
struct ShaderStats {
    bool     valid = false;
    // DXIL STAT has a different layout than DXBC STAT.
    // We store raw dwords and interpret what we can.
    uint32_t instructionCount  = 0;
    uint32_t tempRegisterCount = 0;
    uint32_t floatOps          = 0;
    uint32_t intOps            = 0;
    uint32_t uintOps           = 0;
    uint32_t textureOps        = 0;
};

// ── DXIL sub-header ────────────────────────────────────────────────────────
struct DxilInfo {
    bool     valid = false;
    uint8_t  majorVersion = 0;
    uint8_t  minorVersion = 0;
    uint32_t bitcodeSize  = 0;
};

// ── Full parsed shader result ──────────────────────────────────────────────
struct GOWRShaderData {
    // GOW custom header (28 bytes)
    uint16_t    formatVersion = 0;
    uint16_t    subVersion    = 0;
    uint32_t    dxbcSize      = 0;
    uint32_t    psoFlags      = 0;
    std::string stageTag;         // "vs", "ps", "hs", "ds", "cs", "ls"
    uint32_t    variantId     = 0;

    // DXBC container
    bool        hasDxbc       = false;
    uint8_t     md5[16]       = {};
    uint32_t    dxbcVersion   = 0;

    // Chunks
    std::vector<DxbcChunk> chunks;

    // Parsed signatures
    std::vector<SignatureElement> inputs;    // from ISG1/ISGN
    std::vector<SignatureElement> outputs;   // from OSG1/OSGN
    std::vector<SignatureElement> patch;     // from PSG1

    // Debug name (from ILDN chunk)
    std::string debugPath;

    // Shader hash (from HASH chunk)
    uint8_t     shaderHash[20] = {};
    bool        hasHash = false;

    // DXIL info
    DxilInfo    dxil;

    // Statistics
    ShaderStats stats;

    // Stage display name
    const char* StageName() const;

    // Component type display name
    static const char* ComponentTypeName(uint32_t type);
    // System value display name
    static const char* SystemValueName(uint32_t sv);
    // Mask → "xyzw" string
    static std::string MaskString(uint8_t mask);
};

// Parse a GOWR shader file (28-byte GOW header + DXBC container)
std::unique_ptr<GOWRShaderData> GOWRShaderParse(std::shared_ptr<IFile> file);

} // namespace GOW
