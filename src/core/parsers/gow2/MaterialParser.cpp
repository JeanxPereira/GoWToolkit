#include "MaterialParser.h"
#include "core/vfs/SliceFile.h"
#include "core/Logger.h"
#include <cstring>

namespace GOW {

// Helper para ler strings de tamanho fixo (O MSVC precisa desta definição antes do uso)
static std::string ReadFixedString(const uint8_t* buf, size_t maxLen) {
    size_t len = strnlen((const char*)buf, maxLen);
    return std::string((const char*)buf, len);
}

// Helper para ler floats raw do buffer
static float ReadFloat(const uint8_t* buf, size_t offset) {
    uint32_t val = *(uint32_t*)(buf + offset);
    return *reinterpret_cast<float*>(&val);
}

std::unique_ptr<GOW2MaterialParser::MaterialData> GOW2MaterialParser::Parse(const ParsedEntry& matEntry, const std::shared_ptr<IFile>& fileSource) {
    if (!fileSource) return nullptr;

    // Header MAT tem 0x38 bytes
    if (matEntry.size < 0x38) return nullptr;

    std::vector<uint8_t> buf(matEntry.size);
    SliceFile slice(fileSource, matEntry.offset, matEntry.size);
    slice.Seek(0, SEEK_SET);
    slice.Read(buf.data(), matEntry.size);

    uint32_t magic = *(uint32_t*)(buf.data());
    if (magic != 0x08) return nullptr;

    auto matData = std::make_unique<MaterialData>();

    // Extraindo cor base do material (Header 0x08, 0x0C, 0x10)
    matData->baseColor[0] = ReadFloat(buf.data(), 0x08);
    matData->baseColor[1] = ReadFloat(buf.data(), 0x0C);
    matData->baseColor[2] = ReadFloat(buf.data(), 0x10);
    matData->baseColor[3] = 1.0f;

    uint32_t layerCount = *(uint32_t*)(buf.data() + 0x34);
    if (layerCount > 10) layerCount = 10; // Sanity check

    uint32_t offset = 0x38; // Início das layers
    for (uint32_t i = 0; i < layerCount; i++) {
        if (offset + 0x40 > buf.size()) break;

        MaterialLayer layer;
        const uint8_t* lBuf = buf.data() + offset;

        // Flags de renderização (Equivalente ao mat.go ParsedFlags)
        uint32_t f0 = *(uint32_t*)(lBuf + 0x00);
        layer.hasTexture = (f0 >> 7) & 1;
        layer.filterLinear = (f0 >> 16) & 1;
        layer.disableDepthWrite = (f0 >> 19) & 1;

        // Rendering Methods (Bits 24-27 do f0)
        if ((f0 >> 27) & 1)      layer.renderingMethod = 1; // Additive
        else if ((f0 >> 26) & 1) layer.renderingMethod = 0; // Usual/Normal
        else if ((f0 >> 25) & 1) layer.renderingMethod = 2; // Subtract
        else if ((f0 >> 24) & 1) layer.renderingMethod = 3; // Strange Blended
        else layer.renderingMethod = 0;

        // Texture Name
        if (layer.hasTexture) {
            layer.textureName = ReadFixedString(lBuf + 16, 24);
        }

        // Layer Blend Color (Offset 0x28 dentro da layer)
        layer.blendColor[0] = ReadFloat(lBuf, 0x28);
        layer.blendColor[1] = ReadFloat(lBuf, 0x2C);
        layer.blendColor[2] = ReadFloat(lBuf, 0x30);
        layer.blendColor[3] = ReadFloat(lBuf, 0x34);

        // Flags de Animação (GameFlags 0x3C)
        uint32_t gFlags = *(uint32_t*)(lBuf + 0x3C);
        layer.uvAnimEnabled = (gFlags & 1) != 0;
        layer.colorAnimEnabled = (gFlags & 2) != 0;

        matData->layers.push_back(layer);
        offset += 0x40;
    }

    return matData;
}

} // namespace GOW