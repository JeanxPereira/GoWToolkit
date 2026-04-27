#pragma once
#include "../shared/AnimationData.h"
#include <cstdint>
#include <memory>

namespace GOW {

class GOW2AnimationParser {
public:
    /// Parse animation data from raw bytes (magic 0x00000003)
    /// Works for both GOW1 and GOW2 — the format is identical.
    static std::unique_ptr<AnimationData> Parse(const uint8_t* data, size_t size);
};

} // namespace GOW
