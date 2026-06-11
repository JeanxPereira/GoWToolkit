#pragma once
#include "Core/Vfs/IFile.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace Onyx {

class GOW2VagParser {
public:
    struct VagData {
        std::vector<int16_t> pcmData;
        uint32_t sampleRate = 22050;
        uint8_t channels = 1;
    };

    static std::unique_ptr<VagData> Parse(const std::shared_ptr<Vfs::IFile>& file);
};

} // namespace Onyx
