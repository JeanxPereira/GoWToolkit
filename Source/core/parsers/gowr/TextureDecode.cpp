#include "TextureDecode.h"
#include "Rdna2Detiler.h"
// This translation unit owns the bcdec implementation.
#define ONYX_BCDEC_IMPLEMENTATION
#include "BcDecoder.h"
#include <chrono>
#include <cstring>
#include <vector>

namespace Onyx {

namespace {

// AGC data_format -> block-compression format. 0x38 is uncompressed RGBA8,
// which TX_regionidmap pins: 200x200 with a data extent of exactly 160000.
constexpr uint32_t kFmtRgba8 = 0x38;

bool BcFormatFor(uint32_t dataFmt, BcFormat& out) {
    switch (dataFmt) {
        case 0x29: case 0x2A: out = BcFormat::BC1; return true;
        case 0x2F:            out = BcFormat::BC4; return true;
        case 0x31:            out = BcFormat::BC5; return true;
        case 0x35: case 0x36: out = BcFormat::BC7; return true;
        default: return false;
    }
}

// Reads mip 0 at `dataOffset`, detiles it when the swizzle mode calls for it,
// and decompresses to RGBA8.
bool DecodeMip0(const std::shared_ptr<Vfs::IFile>& file, int64_t dataOffset,
                uint32_t width, uint32_t height, uint32_t dw1, uint32_t dw3,
                const std::string& name,
                Parsers::TextureData& out, std::string& error) {
    const uint32_t swMode      = (dw3 >> 20) & 0x1F;
    const uint32_t pipeBankXor = (dw3 >> 8)  & 0x3FFF;
    const uint32_t dataFmt     = (dw1 >> 20) & 0x3F;

    std::vector<uint8_t> rgba;

    if (dataFmt == kFmtRgba8) {
        rgba.resize((size_t)width * height * 4);
        file->Seek(dataOffset, 0);
        if (file->Read(rgba.data(), rgba.size()) != rgba.size()) {
            error = "short read on RGBA8 texture";
            return false;
        }
    } else {
        BcFormat fmt;
        if (!BcFormatFor(dataFmt, fmt)) {
            error = "unsupported AGC data_format 0x" + std::to_string(dataFmt);
            return false;
        }

        const uint32_t blockBytes = BcBlockSize(fmt);
        const uint32_t blocksX    = (width  + 3) / 4;
        const uint32_t blocksY    = (height + 3) / 4;
        const size_t   bcSize     = (size_t)blocksX * blocksY * blockBytes;

        std::vector<uint8_t> tiled(bcSize);
        file->Seek(dataOffset, 0);
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

        if (!DecompressBc(linear.data(), bcSize, width, height, fmt, rgba)) {
            error = "BC decompress failed";
            return false;
        }
    }

    out.name             = name;
    out.width            = width;
    out.height           = height;
    out.isCompressed     = false;
    // No out.glInternalFormat: Onyx::Parsers::TextureData (current SDK) has
    // no such field -- its own header comment says the struct is "always
    // RGBA8 output", so there is nothing left to record here. This line
    // used to set the GL_RGBA constant unconditionally (never anything
    // else), which is exactly the fixed contract the struct's comment now
    // documents directly; dropping it changes no decoded output. Pre-
    // existing drift from an earlier SDK pin, first surfaced when Phase 2
    // Task 4 linked this file into gowtoolkit_tests for the first time.
    out.dataSize         = (uint32_t)rgba.size();
    out.pixels           = std::move(rgba);
    return true;
}

} // namespace

bool GOWRDecodeResidentTexture(const std::shared_ptr<Vfs::IFile>& descriptor,
                               const std::shared_ptr<Vfs::IFile>& payload,
                               const std::string& name,
                               Parsers::TextureData& out,
                               std::string& error)
{
    error.clear();
    if (!descriptor || !descriptor->IsValid() || !payload || !payload->IsValid()) {
        error = "missing descriptor or payload entry";
        return false;
    }
    if (descriptor->Size() < 0xC8) { error = "descriptor too small"; return false; }

    uint16_t width = 0, height = 0;
    descriptor->Seek(0x48, SEEK_SET);
    descriptor->Read(&width, 2);
    descriptor->Read(&height, 2);
    if (width == 0 || height == 0) { error = "zero-sized texture"; return false; }

    char gnf[4] = {};
    descriptor->Seek(0x68, SEEK_SET);
    descriptor->Read(gnf, 4);
    if (std::memcmp(gnf, "GNF ", 4) != 0) {
        error = "no GNF block in the descriptor";
        return false;
    }

    // Same T# layout the texpack blocks use, 0x10 past the GNF header.
    uint32_t dw1 = 0, dw3 = 0;
    descriptor->Seek(0x68 + 0x14, SEEK_SET);
    descriptor->Read(&dw1, 4);
    descriptor->Seek(0x68 + 0x1C, SEEK_SET);
    descriptor->Read(&dw3, 4);

    return DecodeMip0(payload, 0, width, height, dw1, dw3, name, out, error);
}
bool GOWRDecodeTexture(TexPackIndex& index,
                       uint64_t hash,
                       const std::string& name,
                       Parsers::TextureData& out,
                       std::string& error)
{
    error.clear();
    if (!hash) { error = "no texture hash"; return false; }

    // The index is built by background tasks that LoadFromGameRoot fires and
    // never joins, so a lookup made before they finish misses every time and
    // the caller reports a texture that is right there as missing.
    //
    // This is the choke point -- every GOWR texture resolves through here --
    // which is why the wait lives here and not at the entry points. Earlier
    // rounds put it in AssetHarness::Load and in the GUI's startup open, and
    // both were incomplete: `decode` and `render` reach Onyx::Cli::Run
    // directly and passed through neither. Returns at once once the index is
    // in, so the per-texture cost after the first is nothing.
    index.WaitUntilLoaded(std::chrono::seconds(60));

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
    // No out.glInternalFormat -- see the identical comment above in this
    // file's other decode path.
    out.dataSize         = static_cast<uint32_t>(rgba.size());
    out.pixels           = std::move(rgba);
    return true;
}

} // namespace Onyx
