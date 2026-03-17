#pragma once
#include <vector>
#include <cstdint>
#include <string>

namespace GOW {

// CPU-side decoded texture: always RGBA8 output
struct TextureData {
    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixels; // RGBA8, row-major, top-to-bottom
    
    bool IsValid() const { return width > 0 && height > 0 && pixels.size() == width * height * 4; }
};

} // namespace GOW
