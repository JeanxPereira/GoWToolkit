#include "Core/Audio/AdpcmDecoder.h"
#include <algorithm>
#include <cmath>

namespace Onyx {

static const double vag_f[5][2] = {
    {0.0, 0.0},
    {60.0 / 64.0, 0.0},
    {115.0 / 64.0, -52.0 / 64.0},
    {98.0 / 64.0, -55.0 / 64.0},
    {122.0 / 64.0, -60.0 / 64.0},
};

size_t AdpcmDecoder::AdpcmSizeToWaveSize(size_t adpcmSize) {
    return (adpcmSize / 16) * 28 * 2;
}

std::vector<int16_t> AdpcmDecoder::Decode(const uint8_t* adpcmData, size_t adpcmSize) {
    if (!adpcmData || adpcmSize == 0 || adpcmSize % 16 != 0)
        return {};

    size_t numBlocks = adpcmSize / 16;
    std::vector<int16_t> result;
    result.reserve(numBlocks * 28);

    double hist1 = 0.0;
    double hist2 = 0.0;

    for (size_t block = 0; block < numBlocks; block++) {
        const uint8_t* blockData = adpcmData + block * 16;

        // Skip fill blocks (0xC0 flag used to pad VPK files)
        if (blockData[0] == 0xC0)
            continue;

        uint32_t predict_nr = (uint32_t)(blockData[0]) >> 4;
        uint32_t shift_factor = (uint32_t)(blockData[0]) & 0xF;

        if (predict_nr > 4) {
            predict_nr = 0;
        }

        double fsamples[28];
        int streamPos = 2; // skip byte 0 (predict/shift) and byte 1 (flags)

        for (int i = 0; i < 28; i += 2) {
            uint8_t sampleByte = blockData[streamPos++];

            // Low nibble first
            int16_t scale = (int16_t)(sampleByte & 0xF) << 12;
            fsamples[i] = (double)(scale >> shift_factor);

            // High nibble second
            scale = (int16_t)(sampleByte & 0xF0) << 8;
            fsamples[i + 1] = (double)(scale >> shift_factor);
        }

        for (int i = 0; i < 28; i++) {
            fsamples[i] = fsamples[i] + hist1 * vag_f[predict_nr][0] + hist2 * vag_f[predict_nr][1];
            hist2 = hist1;
            hist1 = fsamples[i];

            int d = (int)(fsamples[i] + 0.5);
            d = std::clamp(d, -32768, 32767);
            result.push_back((int16_t)d);
        }
    }

    return result;
}

} // namespace Onyx
