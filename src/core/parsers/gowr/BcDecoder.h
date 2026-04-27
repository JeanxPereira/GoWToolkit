#pragma once
// BcDecoder.h — CPU-based BC texture decompression using bcdec
// Decompresses BC1/BC3/BC4/BC5/BC7 → RGBA8 or R8/RG8

#include <cstdint>
#include <vector>
#include <cstring>

#define BCDEC_IMPLEMENTATION
#include "bcdec.h"

namespace GOW {

// Format enum matching GNF format IDs we encounter
enum class BcFormat {
    BC1,   // 8 bytes/block, 4bpp (DXT1)
    BC3,   // 16 bytes/block, 8bpp (DXT5)
    BC4,   // 8 bytes/block, single channel
    BC5,   // 16 bytes/block, two channels
    BC7,   // 16 bytes/block, 8bpp (BPTC)
};

inline uint32_t BcBlockSize(BcFormat fmt) {
    switch (fmt) {
        case BcFormat::BC1: return 8;
        case BcFormat::BC4: return 8;
        default:            return 16;
    }
}

// Decompress a full BC-compressed image to RGBA8.
// Input: linearized (post-detile) BC data
// Output: width * height * 4 bytes (RGBA8)
inline bool DecompressBc(
    const uint8_t* bcData,
    size_t         bcDataSize,
    uint32_t       pixelWidth,
    uint32_t       pixelHeight,
    BcFormat       format,
    std::vector<uint8_t>& outRGBA)
{
    if (!bcData || pixelWidth == 0 || pixelHeight == 0)
        return false;

    uint32_t blocksX = (pixelWidth + 3) / 4;
    uint32_t blocksY = (pixelHeight + 3) / 4;
    uint32_t blockBytes = BcBlockSize(format);
    size_t   expectedSize = (size_t)blocksX * blocksY * blockBytes;

    outRGBA.resize(pixelWidth * pixelHeight * 4, 0);

    // Pitch in bytes for destination (RGBA8, 4 bytes per pixel, 4 pixels per row of block)
    int dstPitch = pixelWidth * 4;

    for (uint32_t by = 0; by < blocksY; ++by) {
        for (uint32_t bx = 0; bx < blocksX; ++bx) {
            size_t blockOff = ((size_t)by * blocksX + bx) * blockBytes;
            if (blockOff + blockBytes > bcDataSize)
                continue;

            const void* block = bcData + blockOff;

            // Output position: top-left pixel of this 4x4 block
            uint32_t px = bx * 4;
            uint32_t py = by * 4;

            // Temporary 4x4 RGBA block
            uint8_t decompressed[4 * 4 * 4]; // 4x4 pixels × 4 bytes

            switch (format) {
                case BcFormat::BC1:
                    bcdec_bc1(block, decompressed, 4 * 4);
                    break;
                case BcFormat::BC3:
                    bcdec_bc3(block, decompressed, 4 * 4);
                    break;
                case BcFormat::BC4: {
                    // BC4 is single channel (R8). Decode to R, then expand to RGBA.
                    uint8_t rBlock[4 * 4];
                    bcdec_bc4(block, rBlock, 4);
                    for (int i = 0; i < 16; ++i) {
                        decompressed[i * 4 + 0] = rBlock[i]; // R
                        decompressed[i * 4 + 1] = rBlock[i]; // G = R (grayscale)
                        decompressed[i * 4 + 2] = rBlock[i]; // B = R
                        decompressed[i * 4 + 3] = 255;        // A
                    }
                    break;
                }
                case BcFormat::BC5: {
                    // BC5 is two channels (RG8). Decode to RG, expand to RGBA.
                    uint8_t rgBlock[4 * 4 * 2];
                    bcdec_bc5(block, rgBlock, 4 * 2);
                    for (int i = 0; i < 16; ++i) {
                        decompressed[i * 4 + 0] = rgBlock[i * 2 + 0]; // R
                        decompressed[i * 4 + 1] = rgBlock[i * 2 + 1]; // G
                        decompressed[i * 4 + 2] = 128;                 // B (neutral for normal maps)
                        decompressed[i * 4 + 3] = 255;                 // A
                    }
                    break;
                }
                case BcFormat::BC7:
                    bcdec_bc7(block, decompressed, 4 * 4);
                    break;
            }

            // Copy 4x4 block to output image, clamping to image bounds
            for (int row = 0; row < 4; ++row) {
                if (py + row >= pixelHeight) break;
                for (int col = 0; col < 4; ++col) {
                    if (px + col >= pixelWidth) break;
                    size_t dstOff = ((py + row) * pixelWidth + (px + col)) * 4;
                    size_t srcOff = (row * 4 + col) * 4;
                    std::memcpy(outRGBA.data() + dstOff,
                                decompressed + srcOff, 4);
                }
            }
        }
    }

    return true;
}

// Convert GNF format ID → BcFormat
inline bool GnfFmtToBc(uint32_t gnfFmt, BcFormat& out) {
    switch (gnfFmt) {
        case 169: case 170: out = BcFormat::BC1; return true;  // BC1 UNORM / SRGB
        case 173: case 174: out = BcFormat::BC3; return true;  // BC3 UNORM / SRGB
        case 175: case 176: out = BcFormat::BC4; return true;  // BC4 UNORM / SNORM
        case 177: case 178: out = BcFormat::BC5; return true;  // BC5 UNORM / SNORM
        case 179: case 180:                                     // BC7 UNORM / SRGB
        case 181: case 182: out = BcFormat::BC7; return true;
        default: return false;
    }
}

} // namespace GOW
