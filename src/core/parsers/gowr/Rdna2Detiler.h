#pragma once
// Rdna2Detiler.h — RDNA2 (GFX10/PS5) texture detiling for GOWToolkit
// Based on AMD AddrLib tiling equations (MIT license, from Mesa/RADV)
// and reverse engineering of libSceAgcTextureTool.dll via Ghidra.
//
// The PS5 / GOWR PC textures use 64KB macro-tile swizzle with XOR-based
// pipe/bank interleaving.  For block-compressed formats (BC1–BC7) each
// "element" is one compressed block (8 or 16 bytes), so the tiling
// operates on a grid of blocks, not pixels.
//
// References:
//   Mesa src/amd/addrlib/src/gfx10/gfx10addrlib.cpp
//   Mesa src/amd/common/ac_surface.c
//   Ghidra analysis of libSceAgcTextureTool.dll (tile copy kernels)

#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

namespace GOW {

// ──────────────────────────────────────────────────────────────────────
// GFX10 / RDNA2 micro-tile dimensions
// For 2D surfaces with 64KB macro blocks:
//
//   Element size │ Micro-tile (elements)
//   ─────────────┼──────────────────────
//     1 byte     │ 256 × 256
//     2 bytes    │ 256 × 128
//     4 bytes    │ 128 × 128
//     8 bytes    │ 128 ×  64   ← BC1/BC4 (8B blocks)
//    16 bytes    │  64 ×  64   ← BC3/BC5/BC7 (16B blocks)
//
// Inside each 256-byte micro-tile, elements follow the standard
// "S" (Standard) swizzle: interleaved X/Y bits via Morton order.
// ──────────────────────────────────────────────────────────────────────

struct Rdna2TileInfo {
    uint32_t microTileW;  // micro-tile width  in elements
    uint32_t microTileH;  // micro-tile height in elements
    uint32_t macroTileW;  // macro-tile width  in elements (64KB / microTile / elemBytes)
    uint32_t macroTileH;  // macro-tile height in elements
    uint32_t elemBytes;   // bytes per element (block)
};

// ── Bit interleave helpers (Morton / Z-order for micro-tiles) ────────

inline uint32_t MortonInterleaveX(uint32_t x, uint32_t y) {
    // For 256B micro-tile in Standard swizzle:
    // Address bits = y3 x3 y2 x2 y1 x1 y0 x0
    // This interleaves the low bits of x and y.
    uint32_t result = 0;
    for (int bit = 0; bit < 8; ++bit) {
        result |= ((x >> bit) & 1) << (2 * bit);
        result |= ((y >> bit) & 1) << (2 * bit + 1);
    }
    return result;
}

// ── Compute 64KB Standard Swizzle address ───────────────────────────
// This implements the ADDR_SW_64KB_S equation for 2D surfaces.
// 
// The 64KB block is organized as:
//   Bits [0..7]   = micro-tile offset (256B, Morton-ordered)
//   Bits [8..15]  = macro-tile row/col within 64KB block
//
// For pipe/bank XOR modes (_S_X, _D_X), an additional XOR is applied
// to bits [8..15] using (y >> shift) to decorrelate cache lines.
// We set pipeBankXor = 0 for now (can be refined later).
//
inline uint64_t ComputeTiledOffset(
    uint32_t x, uint32_t y,
    uint32_t pitchInElements,
    uint32_t heightInElements,
    uint32_t elemBytes,
    uint32_t pipeBankXor = 0,
    uint32_t macroBlockBytes = 65536)
{
    // Determine micro-tile dimensions based on element size
    // Each micro-tile is 256 bytes.
    uint32_t microW, microH;
    switch (elemBytes) {
        case 1:  microW = 16; microH = 16; break;
        case 2:  microW = 16; microH = 8;  break;
        case 4:  microW = 8;  microH = 8;  break;
        case 8:  microW = 8;  microH = 4;  break;   // BC1/BC4
        case 16: microW = 4;  microH = 4;  break;   // BC3/BC5/BC7
        default: microW = 8;  microH = 4;  break;
    }

    // Position within the micro-tile
    uint32_t microX = x & (microW - 1);
    uint32_t microY = y & (microH - 1);

    // Morton encoding within micro-tile → byte offset [0..255]
    uint32_t microOffset = MortonInterleaveX(microX, microY) * elemBytes;

    // How many micro-tiles fit in one macro-block?
    uint32_t microTilesPerMacro = macroBlockBytes / 256;
    
    // Grid of micro-tiles within one macro-block
    // sqrt for square grid: 256→16x16, 16→4x4, 1→1x1
    uint32_t macroTilesW = 1, macroTilesH = 1;
    {
        uint32_t n = microTilesPerMacro;
        macroTilesW = 1;
        while (macroTilesW * macroTilesW < n) macroTilesW++;
        macroTilesH = n / macroTilesW;
    }

    uint32_t tileX = x / microW;
    uint32_t tileY = y / microH;

    // Local tile position within the macro-block
    uint32_t localTileX = tileX & (macroTilesW - 1);
    uint32_t localTileY = tileY & (macroTilesH - 1);

    // Tile index within macro-block (Morton order)
    uint32_t tileIndex = MortonInterleaveX(localTileX, localTileY);

    // Within-block byte offset
    uint32_t blkOffset = tileIndex * 256 + microOffset;
    
    // Apply pipeBankXor: XOR on bits [8..N] of the block offset
    // Matches AddrLib: pipeBankXor << pipeInterleaveLog2 (=8 for 256B interleave)
    uint32_t blkSizeLog2 = 0;
    { uint32_t sz = macroBlockBytes; while (sz > 1) { sz >>= 1; blkSizeLog2++; } }
    uint32_t blkMask = (1u << blkSizeLog2) - 1;
    uint32_t xorValue = (pipeBankXor << 8) & blkMask;
    blkOffset ^= xorValue;

    // Macro-block position
    uint32_t macroBlockX = tileX / macroTilesW;
    uint32_t macroBlockY = tileY / macroTilesH;

    // Pitch in macro-blocks
    uint32_t pitchInMicroTiles = (pitchInElements + microW - 1) / microW;
    uint32_t pitchInMacroBlocks = (pitchInMicroTiles + macroTilesW - 1) / macroTilesW;

    uint64_t macroBlockIndex = (uint64_t)macroBlockY * pitchInMacroBlocks + macroBlockX;
    uint64_t offset = macroBlockIndex * (uint64_t)macroBlockBytes + blkOffset;

    return offset;
}

// ── Main detiling function ──────────────────────────────────────────
// Converts a tiled (swizzled) texture buffer to linear layout.
//
// For BC formats:
//   widthInBlocks  = ceil(pixelWidth / 4)
//   heightInBlocks = ceil(pixelHeight / 4)
//   blockBytes     = 8 (BC1/BC4) or 16 (BC3/BC5/BC7)
//
// The pipeline is:  Detile (on blocks) → BC decompress → Upload to GL

inline bool DetileRdna2(
    const uint8_t* tiledSrc,
    size_t         tiledSize,
    uint8_t*       linearDst,
    uint32_t       widthInBlocks,
    uint32_t       heightInBlocks,
    uint32_t       blockBytes,     // 8 for BC1/BC4, 16 for BC3/BC5/BC7
    uint32_t       pipeBankXor = 0,
    uint32_t       macroBlockBytes = 65536)
{
    if (!tiledSrc || !linearDst || widthInBlocks == 0 || heightInBlocks == 0)
        return false;

    // Pad to micro-tile alignment
    uint32_t microW, microH;
    switch (blockBytes) {
        case 8:  microW = 8;  microH = 4;  break;
        case 16: microW = 4;  microH = 4;  break;
        default: microW = 8;  microH = 4;  break;
    }

    uint32_t pitchAligned = (widthInBlocks + microW - 1) & ~(microW - 1);

    size_t linearSize = (size_t)widthInBlocks * heightInBlocks * blockBytes;
    std::memset(linearDst, 0, linearSize);

    for (uint32_t by = 0; by < heightInBlocks; ++by) {
        for (uint32_t bx = 0; bx < widthInBlocks; ++bx) {
            uint64_t tiledOff = ComputeTiledOffset(
                bx, by, pitchAligned, heightInBlocks, blockBytes,
                pipeBankXor, macroBlockBytes);

            size_t linearOff = ((size_t)by * widthInBlocks + bx) * blockBytes;

            if (tiledOff + blockBytes <= tiledSize &&
                linearOff + blockBytes <= linearSize) {
                std::memcpy(linearDst + linearOff,
                            tiledSrc + tiledOff,
                            blockBytes);
            }
        }
    }

    return true;
}

} // namespace GOW
