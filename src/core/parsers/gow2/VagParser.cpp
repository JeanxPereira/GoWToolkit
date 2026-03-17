#include "VagParser.h"
#include "core/audio/AdpcmDecoder.h"
#include "core/Logger.h"
#include <cstring>

namespace GOW {

static uint32_t ReadU32BE(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

std::unique_ptr<GOW2VagParser::VagData> GOW2VagParser::Parse(const std::shared_ptr<IFile>& file) {
    if (!file || !file->IsValid()) return nullptr;

    // VAG header is 0x30 (48) bytes
    uint8_t header[0x30];
    file->Seek(0, SEEK_SET);
    if (file->Read(header, 0x30) != 0x30) {
        LOG_ERR("[VAG] Failed to read header");
        return nullptr;
    }

    // Check magic: "VAGp" (0x56 0x41 0x47 0x70)
    if (header[0] != 0x56 || header[1] != 0x41 || header[2] != 0x47 || header[3] != 0x70) {
        LOG_ERR("[VAG] Invalid magic: 0x%02X%02X%02X%02X", header[0], header[1], header[2], header[3]);
        return nullptr;
    }

    uint32_t dataSize = ReadU32BE(header + 0x0C);
    uint32_t sampleRate = ReadU32BE(header + 0x10);
    uint8_t channels = header[0x1E];

    if (channels == 0) channels = 1;

    LOG_INFO("[VAG] sampleRate=%u, channels=%u, dataSize=%u", sampleRate, channels, dataSize);

    if (dataSize == 0 || dataSize > 100 * 1024 * 1024) {
        LOG_ERR("[VAG] Suspicious data size: %u", dataSize);
        return nullptr;
    }

    // Read ADPCM data after header
    std::vector<uint8_t> adpcmData(dataSize);
    file->Seek(0x30, SEEK_SET);
    size_t bytesRead = file->Read(adpcmData.data(), dataSize);
    if (bytesRead < dataSize) {
        LOG_WARN("[VAG] Read only %zu of %u bytes", bytesRead, dataSize);
        adpcmData.resize(bytesRead);
    }

    auto result = std::make_unique<VagData>();
    result->sampleRate = sampleRate;
    result->channels = channels;
    result->pcmData = AdpcmDecoder::Decode(adpcmData.data(), adpcmData.size());

    LOG_INFO("[VAG] Decoded %zu PCM samples", result->pcmData.size());
    return result;
}

} // namespace GOW
