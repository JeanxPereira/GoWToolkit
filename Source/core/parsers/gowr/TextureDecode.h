#pragma once
#include "TexPackIndex.h"
#include <Onyx/Parsers/TextureData.h>
#include <cstdint>
#include <string>

namespace Onyx {

// Decodes one GOWR texture from the texpack into RGBA8.
//
// The path is: locate the block by hash, read the AGC T# descriptor for its
// swizzle mode and block-compression format, detile, then decompress. Mip 0
// sits at the end of the block, because mips are stored smallest first.
//
// Shared by the texture viewer and the material pipeline so both agree on the
// format mapping; a second copy of this would drift.
//
// Returns false with a short reason in `error` when the texture cannot be
// decoded - an unknown swizzle mode or data format is expected on some assets
// and is not a failure of the caller.
// Decodes a texture that never streams: its descriptor declares a streamed size
// of zero, so the WAD payload is the whole image and no texpack holds it.
//
// The pixel format is not lost with the texpack - a descriptor embeds its own
// GNF block at 0x68, the same structure the texpack blocks carry. Mip 0 sits at
// the start here, which TX_regionidmap confirms: its data extent is exactly
// width * height * 4.
//
// `descriptor` is the type 0x0022 entry, `payload` the 0x80A2 one.
bool GOWRDecodeResidentTexture(const std::shared_ptr<Vfs::IFile>& descriptor,
                               const std::shared_ptr<Vfs::IFile>& payload,
                               const std::string& name,
                               Parsers::TextureData& out,
                               std::string& error);

bool GOWRDecodeTexture(TexPackIndex& index,
                       uint64_t hash,
                       const std::string& name,
                       Parsers::TextureData& out,
                       std::string& error);

} // namespace Onyx
