#include "VpkParser.h"
#include "core/audio/AdpcmDecoder.h"
#include "core/Logger.h"
#include <cstring>

namespace GOW {

static uint32_t ReadU32LE(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

std::unique_ptr<GOW2VpkParser::VpkData> GOW2VpkParser::Parse(const std::shared_ptr<IFile>& file) {
    if (!file || !file->IsValid()) return nullptr;

    // VPK header is 0x20 (32) bytes
    uint8_t header[0x20];
    file->Seek(0, SEEK_SET);
    if (file->Read(header, 0x20) != 0x20) {
        LOG_ERR("[VPK] Failed to read header");
        return nullptr;
    }

    uint32_t dataSize = ReadU32LE(header + 0x04);
    uint32_t sampleRate = ReadU32LE(header + 0x10);
    uint32_t channels = ReadU32LE(header + 0x14);

    if (channels == 0) channels = 1;
    if (channels > 4) {
        LOG_ERR("[VPK] Too many channels: %u", channels);
        return nullptr;
    }

    LOG_INFO("[VPK] sampleRate=%u, channels=%u, dataSize=%u (per channel)", sampleRate, channels, dataSize);

    if (dataSize == 0 || dataSize > 100 * 1024 * 1024) {
        LOG_ERR("[VPK] Suspicious data size: %u", dataSize);
        return nullptr;
    }

    const uint32_t SECTOR_SIZE = 2048;
    const uint32_t BLOCK_SIZE = 0x1000; // 4096 bytes per channel per block
    uint32_t totalBlockSize = BLOCK_SIZE * channels;

    file->Seek(SECTOR_SIZE, SEEK_SET);

    // Decode each channel separately
    std::vector<std::vector<int16_t>> channelPcm(channels);

    uint32_t remaining = dataSize;
    while (remaining > 0) {
        uint32_t chunkSize = std::min(BLOCK_SIZE, remaining);

        for (uint32_t ch = 0; ch < channels; ch++) {
            std::vector<uint8_t> blockData(chunkSize);
            size_t bytesRead = file->Read(blockData.data(), chunkSize);
            if (bytesRead == 0) goto done;
            blockData.resize(bytesRead);

            auto decoded = AdpcmDecoder::Decode(blockData.data(), blockData.size());
            channelPcm[ch].insert(channelPcm[ch].end(), decoded.begin(), decoded.end());
        }

        remaining -= chunkSize;
    }
done:

    auto result = std::make_unique<VpkData>();
    result->sampleRate = sampleRate;
    result->channels = channels;

    if (channels == 1) {
        result->pcmData = std::move(channelPcm[0]);
    } else {
        // Interleave channels
        size_t maxSamples = 0;
        for (const auto& ch : channelPcm)
            maxSamples = std::max(maxSamples, ch.size());

        result->pcmData.resize(maxSamples * channels, 0);
        for (size_t i = 0; i < maxSamples; i++) {
            for (uint32_t ch = 0; ch < channels; ch++) {
                if (i < channelPcm[ch].size())
                    result->pcmData[i * channels + ch] = channelPcm[ch][i];
            }
        }
    }

    LOG_INFO("[VPK] Decoded %zu total PCM samples (%u channels)", result->pcmData.size(), channels);
    return result;
}

} // namespace GOW
