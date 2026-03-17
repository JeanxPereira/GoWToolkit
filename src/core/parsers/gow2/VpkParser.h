#pragma once
#include "core/vfs/IFile.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace GOW {

class GOW2VpkParser {
public:
    struct VpkData {
        std::vector<int16_t> pcmData; // interleaved if multi-channel
        uint32_t sampleRate = 22050;
        uint32_t channels = 1;
    };

    static std::unique_ptr<VpkData> Parse(const std::shared_ptr<IFile>& file);
};

} // namespace GOW
