#pragma once
// Rdna2Detiler.h — RDNA2 / GFX10 texture detiling (PS5 AGC)
//
// Detile equation derived from libSceAgcTextureTool.dll (AMD AddrLib gfx10).
// Each tiled byte/element offset is a bit-permutation of (X, Y) coordinates,
// optionally XOR'd with a pipeBankXor pattern for the _X variants.
//
// Coordinates are in *elements*, where one element = one BC block (8B or 16B)
// for compressed formats, or one texel for uncompressed.
//
// Equation form (per AddrLib):
//   out_addr_bit[i] = XOR of selected (X_bit_j, Y_bit_k, const) terms
//
// For non-XOR modes (sw_mode < 16), the equation is a *pure permutation* —
// each output bit comes from exactly one input bit. We encode it as a 32-entry
// table of BitSrc records. For XOR modes, an extra pipeBankXor mask is XOR'd
// into the macro-block bits.

#include <cstdint>
#include <cstring>
#include <vector>

namespace GOW { namespace Rdna2 {

enum class BitChan : uint8_t { X = 0, Y = 1 };

struct BitSrc {
    BitChan  chan;
    uint8_t  bit;
};

// Per-macro inner equation: 8 bits describing the tile index inside one
// 256-element macro block (= 16×16 elements regardless of element size in
// sw_mode 5). Bits 8+ of the final tile index come from row-major macro
// position computed at runtime, so the equation is independent of image size.
struct Equation {
    BitSrc   inner[8];     // bits 0..7 of tile index
    uint8_t  macroBlocksW; // macro shape in elements (always 16 for sw_mode 5)
    uint8_t  macroBlocksH;
};

// Equations indexed by (sw_mode, element-bytes log2).
// Element bytes: 8B (BC1/BC4) → log2=3, 16B (BC3/BC5/BC7) → log2=4.
// Empirically derived for GOW Ragnarök PC; verified against DDS samples.
// SW modes: gfx10 5-bit sw_mode field from T# dw3 bits[24:20].
inline const Equation* GetEquation(uint32_t swMode, uint32_t elemBytesLog2) {
    // sw_mode 5, 8B element — micro-tile 8w × 4h, MT cluster 2w × 4h, macro 16×16
    static const Equation EQ_SW5_8B = {{
        {BitChan::X,0},{BitChan::Y,0},{BitChan::Y,1},{BitChan::X,1},
        {BitChan::X,2},{BitChan::Y,2},{BitChan::X,3},{BitChan::Y,3},
    }, 16, 16};

    // sw_mode 5, 16B element — micro-tile 4w × 4h, MT cluster 4w × 4h, macro 16×16
    static const Equation EQ_SW5_16B = {{
        {BitChan::Y,0},{BitChan::Y,1},{BitChan::X,0},{BitChan::X,1},
        {BitChan::Y,2},{BitChan::X,2},{BitChan::Y,3},{BitChan::X,3},
    }, 16, 16};

    if (swMode == 5) {
        if (elemBytesLog2 == 3) return &EQ_SW5_8B;
        if (elemBytesLog2 == 4) return &EQ_SW5_16B;
    }
    return nullptr;
}

// Detile a tiled buffer into linear order.
//   tiled / tiledSize: input tiled element data
//   linear: output buffer of size widthInElems*heightInElems*elemBytes
//   widthInElems / heightInElems: image size in elements (for BC: block count)
//   elemBytes: bytes per element (8 = BC1/BC4, 16 = BC3/BC5/BC7, etc.)
//   swMode: gfx10 5-bit swizzle mode from T# descriptor
//   pipeBankXor: 14-bit pipe/bank XOR field (XOR'd into macro bits for `_X` modes)
//
// Returns true on success. On unknown sw_mode/elem combination, returns false
// and leaves linear buffer untouched.
inline bool Detile(
    const uint8_t* tiled, size_t tiledSize,
    uint8_t*       linear,
    uint32_t       widthInElems,
    uint32_t       heightInElems,
    uint32_t       elemBytes,
    uint32_t       swMode,
    uint32_t       pipeBankXor = 0)
{
    if (!tiled || !linear || widthInElems == 0 || heightInElems == 0)
        return false;

    uint32_t elemBytesLog2 = 0;
    while ((1u << elemBytesLog2) < elemBytes) ++elemBytesLog2;

    const Equation* eq = GetEquation(swMode, elemBytesLog2);
    if (!eq) return false;

    (void)pipeBankXor; // sw_mode < 16 ignores pipeBankXor on this engine.

    const uint32_t macW = eq->macroBlocksW;
    const uint32_t macH = eq->macroBlocksH;
    const uint32_t macrosWide = (widthInElems + macW - 1) / macW;

    for (uint32_t y = 0; y < heightInElems; ++y) {
        const uint32_t macY = y / macH;
        for (uint32_t x = 0; x < widthInElems; ++x) {
            const uint32_t macX = x / macW;
            const uint32_t macroIdx = macY * macrosWide + macX;

            // Inner bits 0..7 from per-format equation (within-macro coords).
            uint32_t inner = 0;
            for (uint8_t i = 0; i < 8; ++i) {
                const uint32_t src = (eq->inner[i].chan == BitChan::X) ? x : y;
                inner |= ((src >> eq->inner[i].bit) & 1u) << i;
            }

            const uint32_t tIdx = (macroIdx << 8) | inner;

            const size_t tOff = static_cast<size_t>(tIdx) * elemBytes;
            const size_t lOff = (static_cast<size_t>(y) * widthInElems + x) * elemBytes;
            if (tOff + elemBytes > tiledSize) continue;
            std::memcpy(linear + lOff, tiled + tOff, elemBytes);
        }
    }
    return true;
}

} // namespace Rdna2

// ── Legacy compatibility shim ────────────────────────────────────────────────
// Old code calls GOW::DetileRdna2(...) with widthInBlocks/heightInBlocks.
// Forward to the new equation-based detiler. Returns false if sw_mode/elem
// combination unknown — caller should fall back gracefully.

inline bool DetileRdna2(
    const uint8_t* tiledSrc, size_t tiledSize,
    uint8_t*       linearDst,
    uint32_t       widthInBlocks,
    uint32_t       heightInBlocks,
    uint32_t       blockBytes,
    uint32_t       /*pipeBankXor*/ = 0,
    uint32_t       /*macroBlockBytes*/ = 65536)
{
    // Legacy path used hardcoded heuristic — replaced by sw_mode=5 default.
    // Callers without sw_mode info get the most common GOWR mode.
    return Rdna2::Detile(tiledSrc, tiledSize, linearDst,
                         widthInBlocks, heightInBlocks, blockBytes,
                         /*swMode=*/5, /*pipeBankXor=*/0);
}

} // namespace GOW
