#include "MaterialParser.h"
#include "core/vfs/SliceFile.h"
#include "core/Logger.h"
#include <cstring>

namespace GOW {

static std::string ReadFixedString(const uint8_t* buf, size_t maxLen) {
    size_t len = strnlen((const char*)buf, maxLen);
    return std::string((const char*)buf, len);
}

std::unique_ptr<GOW2MaterialParser::MaterialData> GOW2MaterialParser::Parse(const ParsedEntry& matEntry, const std::shared_ptr<IFile>& fileSource) {
    if (!fileSource) {
        LOG_ERR("[GOW2Material] fileSource is null for MAT %s", matEntry.name.c_str());
        return nullptr;
    }
    if (matEntry.size < 0x38) { // MAT header is 0x38
        LOG_ERR("[GOW2Material] MAT %s is too small: %zu bytes (need at least 0x38)", matEntry.name.c_str(), matEntry.size);
        return nullptr;
    }

    std::vector<uint8_t> buf(matEntry.size);
    SliceFile slice(fileSource, matEntry.offset, matEntry.size);
    slice.Seek(0, SEEK_SET);
    slice.Read(buf.data(), matEntry.size);

    uint32_t magic = *(uint32_t*)(buf.data());
    if (magic != 0x08) {
        LOG_ERR("[GOW2Material] Invalid MAT magic: 0x%X (expected 0x08) for %s", magic, matEntry.name.c_str());
        return nullptr;
    }

    uint32_t layerCount = *(uint32_t*)(buf.data() + 0x34);
    if (layerCount == 0 || layerCount > 10) { // arbitrary sanity check
        LOG_ERR("[GOW2Material] Suspicious MAT layer count: %u for %s", layerCount, matEntry.name.c_str());
        return nullptr;
    }

    auto matData = std::make_unique<MaterialData>();
    matData->layers.resize(layerCount);

    uint32_t offset = 0x38; // First layer starts here
    for (uint32_t i = 0; i < layerCount; i++) {
        if (offset + 0x40 > buf.size()) {
            LOG_ERR("[GOW2Material] Layer parsing %u overflows buffer in %s", i, matEntry.name.c_str());
            break;
        }

        uint32_t flags0 = *(uint32_t*)(buf.data() + offset + 0);
        bool hasTexture = (flags0 >> 7) & 1;

        if (hasTexture) {
            matData->layers[i].textureName = ReadFixedString(buf.data() + offset + 16, 24);
            matData->layers[i].hasTexture = true;
        }

        offset += 0x40; // LAYER_SIZE
    }

    return matData;
}

} // namespace GOW
