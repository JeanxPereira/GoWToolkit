#include "ShaderParser.h"
#include "core/Logger.h"
#include <cstring>
#include <algorithm>

namespace GOW {

// ── Display helpers ────────────────────────────────────────────────────────

const char* GOWRShaderData::StageName() const {
    if (stageTag == "vs") return "Vertex Shader";
    if (stageTag == "ps") return "Pixel Shader";
    if (stageTag == "hs") return "Hull Shader";
    if (stageTag == "ds") return "Domain Shader";
    if (stageTag == "cs") return "Compute Shader";
    if (stageTag == "ls") return "Library Shader";
    return "Unknown";
}

const char* GOWRShaderData::ComponentTypeName(uint32_t type) {
    switch (type) {
        case 1:  return "uint";
        case 2:  return "int";
        case 3:  return "float";
        default: return "???";
    }
}

const char* GOWRShaderData::SystemValueName(uint32_t sv) {
    switch (sv) {
        case 0:  return "";           // no system value
        case 1:  return "SV_Position";
        case 2:  return "SV_ClipDistance";
        case 3:  return "SV_CullDistance";
        case 4:  return "SV_RenderTargetArrayIndex";
        case 5:  return "SV_ViewportArrayIndex";
        case 6:  return "SV_VertexID";
        case 7:  return "SV_PrimitiveID";
        case 8:  return "SV_InstanceID";
        case 9:  return "SV_IsFrontFace";
        case 10: return "SV_SampleIndex";
        case 11: return "SV_TessFactor";
        case 12: return "SV_InsideTessFactor";
        case 13: return "SV_Target";
        case 14: return "SV_Depth";
        case 15: return "SV_Coverage";
        case 16: return "SV_DepthGE";
        case 17: return "SV_DepthLE";
        case 18: return "SV_StencilRef";
        case 19: return "SV_InnerCoverage";
        case 23: return "SV_DomainLocation";
        case 24: return "SV_OutputControlPointID";
        default: return "SV_???";
    }
}

std::string GOWRShaderData::MaskString(uint8_t mask) {
    std::string s;
    if (mask & 0x1) s += 'x';
    if (mask & 0x2) s += 'y';
    if (mask & 0x4) s += 'z';
    if (mask & 0x8) s += 'w';
    return s.empty() ? "none" : s;
}

// ── Signature parser (ISG1 / OSG1 / PSG1) ─────────────────────────────────
// Layout: uint32 elementCount, uint32 headerSize (always 8),
//         then per element: 6 dwords (24 bytes) for ISGN/OSGN,
//         or 8 dwords (32 bytes) for ISG1/OSG1 (has stream field).
//
// Strings are null-terminated at nameOffset relative to chunk data start.

static bool ParseSignature(const uint8_t* data, uint32_t size,
                           std::vector<SignatureElement>& out, bool isV1)
{
    if (size < 8) return false;

    uint32_t elementCount;
    std::memcpy(&elementCount, data, 4);

    if (elementCount > 256) return false; // sanity check

    const uint32_t elementSize = isV1 ? 32 : 24;
    const uint32_t headerSize  = 8;

    if (size < headerSize + elementCount * elementSize) return false;

    for (uint32_t i = 0; i < elementCount; i++) {
        const uint8_t* elem = data + headerSize + i * elementSize;
        SignatureElement se;

        uint32_t offset = 0;
        if (isV1) {
            // ISG1/OSG1: first dword is stream index (skip for input)
            // uint32_t stream;
            // std::memcpy(&stream, elem, 4);
            uint32_t nameOffset;
            std::memcpy(&nameOffset, elem + 4, 4);
            std::memcpy(&se.semanticIndex,   elem + 8,  4);
            std::memcpy(&se.systemValueType, elem + 12, 4);
            std::memcpy(&se.componentType,   elem + 16, 4);
            std::memcpy(&se.registerIndex,   elem + 20, 4);
            se.mask          = *(elem + 24);
            se.readWriteMask = *(elem + 25);
            offset = nameOffset;
        } else {
            // ISGN/OSGN: no stream field
            uint32_t nameOffset;
            std::memcpy(&nameOffset, elem, 4);
            std::memcpy(&se.semanticIndex,   elem + 4,  4);
            std::memcpy(&se.systemValueType, elem + 8,  4);
            std::memcpy(&se.componentType,   elem + 12, 4);
            std::memcpy(&se.registerIndex,   elem + 16, 4);
            se.mask          = *(elem + 20);
            se.readWriteMask = *(elem + 21);
            offset = nameOffset;
        }

        // Read semantic name string
        if (offset < size) {
            const char* str = reinterpret_cast<const char*>(data + offset);
            size_t maxLen = size - offset;
            se.semanticName = std::string(str, strnlen(str, maxLen));
        }

        out.push_back(std::move(se));
    }
    return true;
}

// ── Main parser ────────────────────────────────────────────────────────────

std::unique_ptr<GOWRShaderData> GOWRShaderParse(std::shared_ptr<IFile> file) {
    if (!file || file->Size() < 0x1C + 4) return nullptr;

    auto shader = std::make_unique<GOWRShaderData>();
    file->Seek(0, 0);

    // ── GOW custom header (28 bytes) ───────────────────────────────────
    file->Read(&shader->formatVersion, 2);
    file->Read(&shader->subVersion, 2);

    uint8_t pad[8];
    file->Read(pad, 8);

    file->Read(&shader->dxbcSize, 4);
    file->Read(&shader->psoFlags, 4);

    char stageBuf[4] = {};
    file->Read(stageBuf, 4);
    shader->stageTag = std::string(stageBuf, strnlen(stageBuf, 4));

    file->Read(&shader->variantId, 4);

    // ── DXBC container ─────────────────────────────────────────────────
    char magic[4] = {};
    file->Read(magic, 4);
    if (magic[0] != 'D' || magic[1] != 'X' || magic[2] != 'B' || magic[3] != 'C') {
        LOG_WARN("[ShaderParser] No DXBC magic at offset 0x1C");
        return shader; // return with GOW header only
    }
    shader->hasDxbc = true;

    file->Read(shader->md5, 16);
    file->Read(&shader->dxbcVersion, 4);

    uint32_t totalSize;
    file->Read(&totalSize, 4);

    uint32_t chunkCount;
    file->Read(&chunkCount, 4);

    if (chunkCount > 64) chunkCount = 64; // sanity

    // Read chunk offsets
    std::vector<uint32_t> chunkOffsets(chunkCount);
    for (uint32_t i = 0; i < chunkCount; i++) {
        file->Read(&chunkOffsets[i], 4);
    }

    // Read the full DXBC payload for chunk parsing
    // Reset to DXBC start and read entire payload
    const int64_t dxbcStart = 0x1C;
    uint32_t payloadSize = std::min(shader->dxbcSize, (uint32_t)(file->Size() - dxbcStart));
    std::vector<uint8_t> payload(payloadSize);
    file->Seek(dxbcStart, 0);
    file->Read(payload.data(), payloadSize);

    // Parse each chunk
    for (uint32_t i = 0; i < chunkCount; i++) {
        if (chunkOffsets[i] + 8 > payloadSize) continue;

        DxbcChunk chunk;
        std::memcpy(chunk.tag, payload.data() + chunkOffsets[i], 4);
        chunk.tag[4] = '\0';
        std::memcpy(&chunk.size, payload.data() + chunkOffsets[i] + 4, 4);
        chunk.offset = chunkOffsets[i];

        const uint8_t* chunkData = payload.data() + chunkOffsets[i] + 8;
        uint32_t chunkDataSize = std::min(chunk.size, payloadSize - chunkOffsets[i] - 8);

        // Parse specific chunk types
        if (std::strncmp(chunk.tag, "ISG1", 4) == 0) {
            ParseSignature(chunkData, chunkDataSize, shader->inputs, true);
        } else if (std::strncmp(chunk.tag, "ISGN", 4) == 0) {
            ParseSignature(chunkData, chunkDataSize, shader->inputs, false);
        } else if (std::strncmp(chunk.tag, "OSG1", 4) == 0) {
            ParseSignature(chunkData, chunkDataSize, shader->outputs, true);
        } else if (std::strncmp(chunk.tag, "OSGN", 4) == 0) {
            ParseSignature(chunkData, chunkDataSize, shader->outputs, false);
        } else if (std::strncmp(chunk.tag, "PSG1", 4) == 0) {
            ParseSignature(chunkData, chunkDataSize, shader->patch, true);
        } else if (std::strncmp(chunk.tag, "ILDN", 4) == 0) {
            // Debug name: null-terminated string
            if (chunkDataSize > 0) {
                shader->debugPath = std::string(
                    reinterpret_cast<const char*>(chunkData),
                    strnlen(reinterpret_cast<const char*>(chunkData), chunkDataSize));
            }
        } else if (std::strncmp(chunk.tag, "HASH", 4) == 0) {
            if (chunkDataSize >= 4 + 16) {
                // Skip 4-byte flags, then 16-byte hash
                std::memcpy(shader->shaderHash, chunkData + 4, 16);
                shader->hasHash = true;
            }
        } else if (std::strncmp(chunk.tag, "DXIL", 4) == 0) {
            if (chunkDataSize >= 12) {
                // Skip 4-byte inner magic, then version info
                shader->dxil.valid = true;
                shader->dxil.majorVersion = chunkData[4];
                shader->dxil.minorVersion = chunkData[5];
                // uint16 at +6 is bitcode offset within chunk
                std::memcpy(&shader->dxil.bitcodeSize, chunkData + 8, 4);
            }
        } else if (std::strncmp(chunk.tag, "STAT", 4) == 0) {
            // STAT layout varies between DXBC and DXIL. For DXIL:
            // First 4 bytes = instruction count (not always reliable).
            if (chunkDataSize >= 4) {
                shader->stats.valid = true;
                std::memcpy(&shader->stats.instructionCount, chunkData, 4);
                if (chunkDataSize >= 8)
                    std::memcpy(&shader->stats.tempRegisterCount, chunkData + 4, 4);
                if (chunkDataSize >= 20)
                    std::memcpy(&shader->stats.floatOps, chunkData + 16, 4);
                if (chunkDataSize >= 24)
                    std::memcpy(&shader->stats.intOps, chunkData + 20, 4);
                if (chunkDataSize >= 28)
                    std::memcpy(&shader->stats.uintOps, chunkData + 24, 4);
                if (chunkDataSize >= 36)
                    std::memcpy(&shader->stats.textureOps, chunkData + 32, 4);
            }
        }

        shader->chunks.push_back(chunk);
    }

    LOG_INFO("[ShaderParser] Parsed %s (%s): %zu chunks, %zu inputs, %zu outputs",
             shader->stageTag.c_str(), shader->StageName(),
             shader->chunks.size(), shader->inputs.size(), shader->outputs.size());

    return shader;
}

} // namespace GOW
