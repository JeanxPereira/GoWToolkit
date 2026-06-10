#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

namespace Onyx {

class AdpcmDecoder {
public:
    // Decode PS2 VAG ADPCM data to 16-bit signed PCM.
    // adpcmData must be a multiple of 16 bytes.
    // Each 16-byte block produces 28 samples.
    static std::vector<int16_t> Decode(const uint8_t* adpcmData, size_t adpcmSize);

    // Calculate output PCM size in bytes from ADPCM size in bytes.
    static size_t AdpcmSizeToWaveSize(size_t adpcmSize);
};

} // namespace Onyx
