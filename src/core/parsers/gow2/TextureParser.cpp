#include "TextureParser.h"
#include "core/vfs/SliceFile.h"
#include "core/Logger.h"
#include <cstring>
#include <algorithm>

namespace GOW {

// ────────────────────────────────────────────────────────────────────────
// PS2 GS Texture Unswizzle
// Converts PS2 VRAM block-interleaved coords to linear offset.
// Direct port from god_of_war_browser/pack/wad/gfx/gfx.go:IndexUnswizzleTexture
// ────────────────────────────────────────────────────────────────────────
uint32_t GOW2TextureParser::IndexUnswizzleTexture(uint32_t x, uint32_t y, uint32_t width) {
    uint32_t block_location = (y & ~0xFu) * width + (x & ~0xFu) * 2;
    uint32_t swap_selector  = (((y + 2) >> 2) & 0x1) * 4;
    uint32_t posY           = (((y & ~3u) >> 1) + (y & 1)) & 0x7;
    uint32_t column_location = posY * width * 2 + ((x + swap_selector) & 0x7) * 4;
    uint32_t byte_num       = ((y >> 1) & 1) + ((x >> 2) & 2);
    return block_location + column_location + byte_num;
}

// ────────────────────────────────────────────────────────────────────────
// PS2 CSM1 Palette Swizzle
// port from gfx.go:IndexSwizzlePalette
// ────────────────────────────────────────────────────────────────────────
int GOW2TextureParser::IndexSwizzlePalette(int i) {
    static const int remap[] = { 0, 2, 1, 3 };
    int blockid  = i / 8;
    int blockpos = i % 8;
    return blockpos + (remap[blockid % 4] + (blockid / 4) * 4) * 8;
}

// ────────────────────────────────────────────────────────────────────────
// Decode palette from a GFX block containing CLUT data
// Returns an array of RGBA uint32 values
// ────────────────────────────────────────────────────────────────────────
std::vector<uint32_t> GOW2TextureParser::DecodePalette(const GFXData& palGfx, int palIdx) {
    if (palIdx >= (int)palGfx.data.size() || palGfx.data[palIdx].empty()) {
        return {};
    }
    
    const auto& palbuf = palGfx.data[palIdx];
    uint32_t colors = palGfx.width * palGfx.realHeight;
    
    std::vector<uint32_t> palette(colors, 0);
    
    for (uint32_t i = 0; i < colors && (i * 4 + 3) < palbuf.size(); i++) {
        uint32_t clr = 
            palbuf[i * 4 + 0] |
            (palbuf[i * 4 + 1] << 8) |
            (palbuf[i * 4 + 2] << 16) |
            (palbuf[i * 4 + 3] << 24);
        
        // Palette swizzle for heights 16 and 32 (CSM1 reordering)
        if (palGfx.height == 16 || palGfx.height == 32) {
            int dest = IndexSwizzlePalette((int)i);
            if (dest < (int)palette.size()) {
                palette[dest] = clr;
            }
        } else {
            palette[i] = clr;
        }
    }
    
    return palette;
}

// ────────────────────────────────────────────────────────────────────────
// Decode pixel indices from a GFX block (PSMT8 with unswizzle, or PSMT4)
// ────────────────────────────────────────────────────────────────────────
std::vector<uint8_t> GOW2TextureParser::DecodePixelIndices(const GFXData& gfx, int dataIdx) {
    if (dataIdx >= (int)gfx.data.size() || gfx.data[dataIdx].empty()) {
        return {};
    }
    
    const auto& data = gfx.data[dataIdx];
    uint32_t w = gfx.width;
    uint32_t h = gfx.realHeight;
    
    std::vector<uint8_t> indices(w * h, 0);
    
    if (gfx.bpi == 8) {
        // PSMT8: 8-bit palette indices
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                if ((gfx.encoding & 2) == 0) {
                    // Needs unswizzle
                    uint32_t pos = IndexUnswizzleTexture(x, y, w);
                    if (pos < (uint32_t)data.size()) {
                        indices[x + y * w] = data[pos];
                    }
                } else {
                    // Linear layout
                    uint32_t pos = x + y * w;
                    if (pos < (uint32_t)data.size()) {
                        indices[x + y * w] = data[pos];
                    }
                }
            }
        }
    } else if (gfx.bpi == 4) {
        // PSMT4: 4-bit palette indices, 2 pixels per byte
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                uint32_t bytePos = (x + y * w) / 2;
                if (bytePos < (uint32_t)data.size()) {
                    uint8_t val = data[bytePos];
                    if ((x & 1) == 0)
                        indices[x + y * w] = val & 0x0F;
                    else
                        indices[x + y * w] = val >> 4;
                }
            }
        }
    } else {
        LOG_ERR("[GOW2Texture] Unsupported bpi: %u", gfx.bpi);
    }
    
    return indices;
}

// ────────────────────────────────────────────────────────────────────────
// Parse a GFX block from raw bytes
// GFX header: magic(4) width(4) height(4) encoding(4) bpi(4) dataCount(4) = 24 bytes
// followed by (width * realHeight * bpi / 8) * dataCount bytes of pixel data
// ────────────────────────────────────────────────────────────────────────
std::unique_ptr<GOW2TextureParser::GFXData> GOW2TextureParser::ParseGFX(const uint8_t* buf, uint32_t bufSize) {
    if (bufSize < 24) return nullptr;
    
    auto gfx = std::make_unique<GFXData>();
    gfx->magic     = *(uint32_t*)(buf + 0);
    gfx->width     = *(uint32_t*)(buf + 4);
    gfx->height    = *(uint32_t*)(buf + 8);
    gfx->encoding  = *(uint32_t*)(buf + 12);
    gfx->bpi       = *(uint32_t*)(buf + 16);
    gfx->dataCount = *(uint32_t*)(buf + 20);
    
    if (gfx->magic != 0x0C) {
        LOG_ERR("[GOW2Texture] GFX bad magic: 0x%X (expected 0x0C)", gfx->magic);
        return nullptr;
    }
    
    if (gfx->dataCount == 0) {
        LOG_ERR("[GOW2Texture] GFX dataCount is 0");
        return nullptr;
    }
    
    gfx->realHeight = gfx->height / gfx->dataCount;
    gfx->dataSize = (gfx->width * gfx->realHeight * gfx->bpi) / 8;
    
    gfx->data.resize(gfx->dataCount);
    uint32_t pos = 24;
    for (uint32_t i = 0; i < gfx->dataCount; i++) {
        if (pos + gfx->dataSize > bufSize) {
            LOG_ERR("[GOW2Texture] GFX data overflows buffer at layer %u", i);
            return nullptr;
        }
        gfx->data[i].assign(buf + pos, buf + pos + gfx->dataSize);
        pos += gfx->dataSize;
    }
    
    LOG_INFO("[GOW2Texture] GFX parsed: %ux%u, bpi=%u, encoding=%u, layers=%u",
        gfx->width, gfx->realHeight, gfx->bpi, gfx->encoding, gfx->dataCount);
    
    return gfx;
}

// ────────────────────────────────────────────────────────────────────────
// TXR Tag format (from txr.go):
//   [0x00] Magic    uint32 (must be 0x07)
//   [0x04] GfxName  char[24]
//   [0x1C] PalName  char[24]
//   [0x34] SubTxr   char[24]
//   [0x4C] LODParamK int32
//   [0x50] LODMult  float32
//   [0x54] Flags    uint32
//   Total = 0x58 bytes
// ────────────────────────────────────────────────────────────────────────

static std::string ReadFixedString(const uint8_t* buf, size_t maxLen) {
    size_t len = strnlen((const char*)buf, maxLen);
    return std::string((const char*)buf, len);
}

// Find a sibling entry by name
static const ParsedEntry* FindSibling(const std::vector<ParsedEntry>& siblings, const std::string& name) {
    for (const auto& e : siblings) {
        if (e.name == name) return &e;
        // Search children recursively
        for (const auto& c : e.children) {
            if (c.name == name) return &c;
        }
    }
    return nullptr;
}

std::unique_ptr<TextureData> GOW2TextureParser::Parse(
    const ParsedEntry& txrEntry,
    const std::vector<ParsedEntry>& siblingEntries,
    const std::shared_ptr<IFile>& fileSource)
{
    if (!fileSource || txrEntry.size < 0x58) {
        LOG_ERR("[GOW2Texture] TXR entry too small: %u bytes", txrEntry.size);
        return nullptr;
    }
    
    // Read TXR tag data
    std::vector<uint8_t> txrBuf(txrEntry.size);
    SliceFile txrSlice(fileSource, txrEntry.offset, txrEntry.size);
    txrSlice.Seek(0, SEEK_SET);
    txrSlice.Read(txrBuf.data(), txrEntry.size);
    
    uint32_t magic = *(uint32_t*)(txrBuf.data() + 0);
    if (magic != 0x07) {
        LOG_ERR("[GOW2Texture] TXR bad magic: 0x%X (expected 0x07)", magic);
        return nullptr;
    }
    
    std::string gfxName = ReadFixedString(txrBuf.data() + 0x04, 24);
    std::string palName = ReadFixedString(txrBuf.data() + 0x1C, 24);
    
    LOG_INFO("[GOW2Texture] TXR '%s': gfx='%s', pal='%s'", txrEntry.name.c_str(), gfxName.c_str(), palName.c_str());
    
    if (gfxName.empty() || palName.empty()) {
        LOG_ERR("[GOW2Texture] TXR has empty gfx/pal names");
        return nullptr;
    }
    
    // Find GFX and PAL sibling entries
    const ParsedEntry* gfxEntry = FindSibling(siblingEntries, gfxName);
    const ParsedEntry* palEntry = FindSibling(siblingEntries, palName);
    
    if (!gfxEntry) {
        LOG_ERR("[GOW2Texture] Cannot find GFX sibling '%s'", gfxName.c_str());
        return nullptr;
    }
    if (!palEntry) {
        LOG_ERR("[GOW2Texture] Cannot find PAL sibling '%s'", palName.c_str());
        return nullptr;
    }
    
    // Read and parse GFX (pixel data)
    std::vector<uint8_t> gfxBuf(gfxEntry->size);
    SliceFile gfxSlice(fileSource, gfxEntry->offset, gfxEntry->size);
    gfxSlice.Seek(0, SEEK_SET);
    gfxSlice.Read(gfxBuf.data(), gfxEntry->size);
    
    auto gfxData = ParseGFX(gfxBuf.data(), gfxEntry->size);
    if (!gfxData) {
        LOG_ERR("[GOW2Texture] Failed to parse GFX '%s'", gfxName.c_str());
        return nullptr;
    }
    
    // Read and parse PAL (palette data)
    std::vector<uint8_t> palBuf(palEntry->size);
    SliceFile palSlice(fileSource, palEntry->offset, palEntry->size);
    palSlice.Seek(0, SEEK_SET);
    palSlice.Read(palBuf.data(), palEntry->size);
    
    auto palData = ParseGFX(palBuf.data(), palEntry->size);
    if (!palData) {
        LOG_ERR("[GOW2Texture] Failed to parse PAL '%s'", palName.c_str());
        return nullptr;
    }
    
    // Decode palette (first layer)
    auto palette = DecodePalette(*palData, 0);
    if (palette.empty()) {
        LOG_ERR("[GOW2Texture] Palette decode failed");
        return nullptr;
    }
    
    // Decode pixel indices (first layer)
    auto indices = DecodePixelIndices(*gfxData, 0);
    if (indices.empty()) {
        LOG_ERR("[GOW2Texture] Pixel index decode failed");
        return nullptr;
    }
    
    // Assemble RGBA output
    uint32_t w = gfxData->width;
    uint32_t h = gfxData->realHeight;
    
    auto result = std::make_unique<TextureData>();
    result->name   = txrEntry.name;
    result->width  = w;
    result->height = h;
    result->pixels.resize(w * h * 4);
    
    for (uint32_t i = 0; i < w * h; i++) {
        uint8_t palIdx = indices[i];
        uint32_t rgba = (palIdx < palette.size()) ? palette[palIdx] : 0xFF00FFFFu;
        
        // Extract RGBA from the PS2 ABGR32 format
        uint8_t r = (uint8_t)(rgba & 0xFF);
        uint8_t g = (uint8_t)((rgba >> 8) & 0xFF);
        uint8_t b = (uint8_t)((rgba >> 16) & 0xFF);
        uint8_t a = (uint8_t)((rgba >> 24) & 0xFF);
        
        // PS2 alpha is 0-128 range, convert to 0-255
        a = (uint8_t)std::min(255, (int)(a * 255.0f / 128.0f));
        
        result->pixels[i * 4 + 0] = r;
        result->pixels[i * 4 + 1] = g;
        result->pixels[i * 4 + 2] = b;
        result->pixels[i * 4 + 3] = a;
    }
    
    LOG_INFO("[GOW2Texture] Decoded '%s': %ux%u RGBA", result->name.c_str(), w, h);
    return result;
}

} // namespace GOW
