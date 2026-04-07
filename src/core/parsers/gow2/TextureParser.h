#pragma once
#include "core/parsers/shared/TextureData.h"
#include "core/WadTypes.h"
#include <memory>
#include <vector>
#include <cstdint>

namespace GOW {

class GOW2TextureParser {
public:
    // Parse a TXR tag and resolve its GFX + PAL siblings to produce RGBA pixels.
    // txrEntry: the TXR_ parsed entry
    // parentEntries: the sibling entries (from the same MDL group) to find GFX/PAL by name
    // wadFileSource: the file source to read raw data from
    static std::unique_ptr<TextureData> Parse(
        const ParsedEntry& txrEntry,
        const std::vector<ParsedEntry>& siblingEntries,
        const std::shared_ptr<IFile>& fileSource);

    // Parse a raw GFX block directly (lower level)
    struct GFXData {
        uint32_t magic;
        uint32_t width;
        uint32_t height;
        uint32_t realHeight;
        uint32_t encoding;
        uint32_t bpi;
        uint32_t dataCount;
        uint32_t dataSize;
        std::vector<std::vector<uint8_t>> data;
    };

    static std::unique_ptr<GFXData> ParseGFX(const uint8_t* buf, uint32_t bufSize);

private:
    // PS2 Texture Unswizzle: maps swizzled VRAM coords to linear coords
    static uint32_t IndexUnswizzleTexture(uint32_t x, uint32_t y, uint32_t width);
    
    // PS2 Palette swizzle: CSM1 palette reordering
    static int IndexSwizzlePalette(int i);
    
    // Decode palette from GFX to RGBA colors
    static std::vector<uint32_t> DecodePalette(const GFXData& palGfx, int palIdx);
    
    // Decode pixel indices from GFX (handles PSMT8 unswizzle and PSMT4 nibble unpacking)
    static std::vector<uint8_t> DecodePixelIndices(const GFXData& gfx, int dataIdx);
};

} // namespace GOW
