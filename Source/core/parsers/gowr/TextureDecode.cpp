#include "TextureDecode.h"
#include "Rdna2Detiler.h"
// This translation unit owns the bcdec implementation.
#define ONYX_BCDEC_IMPLEMENTATION
#include "BcDecoder.h"
#include <cstring>
#include <vector>

namespace Onyx {

bool GOWRDecodeTexture(TexPackIndex& index,
                       uint64_t hash,
                       const std::string& name,
                       Parsers::TextureData& out,
                       std::string& error)
{
    error.clear();
    if (!hash) { error = "no texture hash"; return false; }

    TexpackEntry entry;
    if (!index.FindTexture(hash, entry)) { error = "hash not in texpack index"; return false; }

    auto file = index.GetFile(entry.packIdx);
    if (!file || !file->IsValid()) { error = "texpack file unavailable"; return false; }

    const uint32_t width  = entry.width;
    const uint32_t height = entry.height;
    if (width == 0 || height == 0) { error = "zero-sized texture"; return false; }

    // Block layout: 16B header, 256B GNF descriptor, then tiled data.
    file->Seek(static_cast<int64_t>(entry.blockDataOffset), 0);
    uint32_t bMagic = 0, bDataOff = 0, bLen = 0, bUnk = 0;
    file->Read(&bMagic, 4);
    file->Read(&bDataOff, 4);
    file->Read(&bLen, 4);
    file->Read(&bUnk, 4);

    uint8_t gnf[0x100] = {};
    file->Read(gnf, sizeof(gnf));

    // AGC T# at +0x10, eight dwords.
    //   dw1 bits[25:20] = data_format
    //   dw3 bits[24:20] = swizzle mode, bits[21:8] = pipeBankXor
    uint32_t dw1 = 0, dw3 = 0;
    std::memcpy(&dw1, gnf + 0x14, 4);
    std::memcpy(&dw3, gnf + 0x1C, 4);

    const uint32_t swMode      = (dw3 >> 20) & 0x1F;
    const uint32_t pipeBankXor = (dw3 >> 8)  & 0x3FFF;
    const uint32_t dataFmt     = (dw1 >> 20) & 0x3F;

    BcFormat fmt;
    switch (dataFmt) {
        case 0x29: case 0x2A: fmt = BcFormat::BC1; break;   // SRGB / UNORM
        case 0x2F:            fmt = BcFormat::BC4; break;   // ATI1
        case 0x31:            fmt = BcFormat::BC5; break;   // ATI2
        case 0x35: case 0x36: fmt = BcFormat::BC7; break;   // SRGB / UNORM
        default:
            error = "unsupported AGC data_format 0x" +
                    std::to_string(dataFmt);
            return false;
    }

    const uint32_t blockBytes = BcBlockSize(fmt);
    const uint32_t blocksX    = (width  + 3) / 4;
    const uint32_t blocksY    = (height + 3) / 4;
    const size_t   bcSize     = static_cast<size_t>(blocksX) * blocksY * blockBytes;

    // Mips run smallest to largest, so mip 0 is the final slice of the block.
    const int64_t mip0 = static_cast<int64_t>(entry.blockDataOffset)
                       + static_cast<int64_t>(bDataOff)
                       + static_cast<int64_t>(entry.rawSize)
                       - static_cast<int64_t>(bcSize);
    if (mip0 < 0) { error = "block smaller than its own mip 0"; return false; }

    std::vector<uint8_t> tiled(bcSize);
    file->Seek(mip0, 0);
    if (file->Read(tiled.data(), tiled.size()) != tiled.size()) {
        error = "short read on texture block";
        return false;
    }

    std::vector<uint8_t> linear(bcSize, 0);
    if (swMode == 0) {
        std::memcpy(linear.data(), tiled.data(), bcSize);
    } else if (!Rdna2::Detile(tiled.data(), tiled.size(), linear.data(),
                              blocksX, blocksY, blockBytes, swMode, pipeBankXor)) {
        error = "no detile equation for sw_mode " + std::to_string(swMode);
        return false;
    }

    std::vector<uint8_t> rgba;
    if (!DecompressBc(linear.data(), bcSize, width, height, fmt, rgba)) {
        error = "BC decompress failed";
        return false;
    }

    out.name             = name;
    out.width            = width;
    out.height           = height;
    out.isCompressed     = false;
    out.glInternalFormat = 0x1908;   // GL_RGBA
    out.dataSize         = static_cast<uint32_t>(rgba.size());
    out.pixels           = std::move(rgba);
    return true;
}

} // namespace Onyx
