#pragma once
#include "core/WadTypes.h"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace GOW {

class GOW2SoundParser {
public:
    struct SoundInfo {
        std::string name;
        uint32_t    streamId = 0;     // Index/offset depending on mode
        uint32_t    adpcmOffset = 0;  // Offset within bankStreamData
        uint32_t    adpcmSize = 0;    // Size of ADPCM data
        uint32_t    sampleRate = 22050;
        bool        hasData = false;
    };

    struct SoundBankData {
        std::vector<SoundInfo>  sounds;
        std::vector<uint8_t>    bankStreamData; // Raw ADPCM stream block
        uint32_t                defaultSampleRate = 22050;
    };

    static std::unique_ptr<SoundBankData> Parse(
        const ParsedEntry& entry,
        const std::shared_ptr<IFile>& fileSource);

private:
    static constexpr uint32_t GOW2_SBP_MAGIC = 0x15;
};

} // namespace GOW
