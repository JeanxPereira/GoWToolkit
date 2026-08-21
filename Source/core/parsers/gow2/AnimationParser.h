#pragma once
#include <Onyx/Parsers/AnimationData.h>
#include <cstdint>
#include <memory>

namespace Onyx {

class GOW2AnimationParser {
public:
    /// Parse animation data from raw bytes (magic 0x00000003)
    /// Works for both GOW1 and GOW2 — the format is identical.
    static std::unique_ptr<Parsers::AnimationData> Parse(const uint8_t* data, size_t size);
};

} // namespace Onyx
