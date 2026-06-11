#pragma once
#include <vector>
#include <cstdint>
#include <string>

namespace Onyx {

// CPU-side decoded texture: always RGBA8 output
struct TextureData {
    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixels; // Raw pixel data
    
    bool isCompressed = false;
    uint32_t glInternalFormat = 0x8058; // GL_RGBA8 by default
    
    // Optional additional dimensions for block compressed
    uint32_t dataSize = 0;
    
    bool IsValid() const { return width > 0 && height > 0 && !pixels.empty(); }
};

} // namespace Onyx
