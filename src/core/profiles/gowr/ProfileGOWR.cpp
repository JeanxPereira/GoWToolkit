#include "ProfileGOWR.h"
#include "GOWRTypes.h"
#include "WadNodeBuilder.h"
#include "../../vfs/IsoFileSystem.h"
#include "../../vfs/MemoryFile.h"
#include "../../Logger.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <map>
#include <lz4frame.h>

// ── ProfileGOWR.cpp ────────────────────────────────────────────────────────
// Binary structures (GOWRWadHeader, GOWRFileDesc, GOWRTypeToString) have been
// moved to GOWRTypes.h so that WadNodeBuilder.cpp can share them.

namespace GOW {

ProfileGOWR::ProfileGOWR() {}

bool ProfileGOWR::Detect(const std::filesystem::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != ".wad") return false;

    // Peek at magic: WTOC (0x434F5457) or LZ4 frame (0x184D2204)
    std::ifstream fs(path, std::ios::binary);
    if (!fs) return false;
    uint32_t magic = 0;
    fs.read(reinterpret_cast<char*>(&magic), 4);
    return magic == 0x434F5457 || magic == 0x184D2204;
}

std::shared_ptr<IVirtualFileSystem> ProfileGOWR::MountArchive(
    const std::filesystem::path& path)
{
    // Ragnarök doesn't use ISO mounting
    return nullptr;
}

bool ProfileGOWR::ParseWad(std::shared_ptr<IFile> file, OpenWad& outWad) {
    if (!file || !file->IsValid()) return false;

    file->Seek(0, SEEK_END);
    int64_t fileSize = file->Tell();
    file->Seek(0, SEEK_SET);

    uint32_t initialMagic = 0;
    file->Read(&initialMagic, 4);

    std::shared_ptr<IFile> parsedFile = file;

    // ── LZ4 decompression ─────────────────────────────────────────────────
    if (initialMagic == 0x184D2204) {
        LOG_INFO("[GOWR] WAD is LZ4-compressed. Decompressing...");

        size_t dstCapacity = 0;
        file->Seek(0x06, SEEK_SET);
        file->Read(&dstCapacity, 4);

        if (dstCapacity == 0 || dstCapacity > 1024ULL * 1024 * 500) {
            LOG_ERR("[GOWR] Invalid decompressed size: %zu", dstCapacity);
            return false;
        }

        std::vector<uint8_t> src(fileSize);
        file->Seek(0, SEEK_SET);
        file->Read(src.data(), fileSize);

        std::vector<uint8_t> dst(dstCapacity);

        LZ4F_dctx* ctx = nullptr;
        LZ4F_createDecompressionContext(&ctx, LZ4F_getVersion());
        LZ4F_decompressOptions_t opn = { 0, 0, 0, 0 };

        size_t srcSize = fileSize;
        size_t outSize = dstCapacity;
        LZ4F_decompress(ctx, dst.data(), &outSize, src.data(), &srcSize, &opn);
        LZ4F_freeDecompressionContext(ctx);

        parsedFile = std::make_shared<GOW::MemoryFile>(std::move(dst));
        fileSize   = static_cast<int64_t>(outSize);
        LOG_INFO("[GOWR] Decompressed to %lld bytes.", fileSize);
    }

    parsedFile->Seek(0, SEEK_SET);

    // ── Read WTOC header ──────────────────────────────────────────────────
    GOWRWadHeader header;
    if (parsedFile->Read(&header, sizeof(GOWRWadHeader)) != sizeof(GOWRWadHeader)) {
        LOG_ERR("[GOWR] Failed to read WAD header");
        return false;
    }

    if (header.magic != 0x434F5457) {
        LOG_ERR("[GOWR] Invalid magic: 0x%08X (expected WTOC = 0x434F5457)", header.magic);
        return false;
    }
    if (header.ver != 0x2) {
        LOG_ERR("[GOWR] Unsupported version: %u (expected 2)", header.ver);
        return false;
    }
    if (header.fileCount == 0) {
        LOG_INFO("[GOWR] WAD contains 0 files.");
        return true;
    }

    LOG_INFO("[GOWR] WAD header: %u files, block0=%u, block1=%u, block2=%u",
        header.fileCount, header.block0Size, header.block1Size, header.block2Size);

    // ── Read all FileDesc entries ─────────────────────────────────────────
    std::vector<GOWRFileDesc> fileDescs(header.fileCount);
    for (uint32_t i = 0; i < header.fileCount; i++) {
        if (parsedFile->Read(&fileDescs[i], sizeof(GOWRFileDesc)) != sizeof(GOWRFileDesc)) {
            LOG_ERR("[GOWR] Failed to read file descriptor %u", i);
            return false;
        }
    }

    // ── Compute absolute offsets via blockBitSet/flush-queue algorithm ────
    // Ported from GOWTool (Wad.cpp). This must run before WadNodeBuilder
    // because it resolves the correct in-file offset for each entry.
    std::vector<size_t> absOffsets(header.fileCount, 0);
    {
        size_t readOff = parsedFile->Tell();  // base = after header + all descs
        std::map<uint8_t, uint32_t>              bitsetOffs;
        std::map<uint8_t, std::vector<uint32_t>> flushQ;

        for (uint32_t i = 0; i < header.fileCount; i++) {
            std::string nameStr(fileDescs[i].name,
                strnlen(fileDescs[i].name, sizeof(fileDescs[i].name)));

            if (fileDescs[i].unk3[0x2] == 1) {
                if (nameStr != "autopad")
                    flushQ[fileDescs[i].blockBitSet].push_back(i);
                if (fileDescs[i].unk2[20] != 0)
                    flushQ[8].push_back(i);

                for (auto& [key, queue] : flushQ) {
                    readOff -= bitsetOffs[key];
                    uint32_t temp = bitsetOffs[key];
                    if (!queue.empty()) {
                        for (auto idx : queue) {
                            if (key == 8 && fileDescs[idx].blockBitSet != 8) {
                                temp = fileDescs[idx].offset2 + 16;
                                bitsetOffs[key] = temp;
                            } else {
                                absOffsets[idx]  = readOff + fileDescs[idx].offset;
                                bitsetOffs[key]  = fileDescs[idx].offset + fileDescs[idx].size;
                                temp             = bitsetOffs[key];
                            }
                        }
                        queue.clear();
                    }
                    readOff += temp;
                }
                if (nameStr == "autopad") {
                    absOffsets[i] = readOff;
                    readOff      += fileDescs[i].size;
                }
            } else {
                flushQ[fileDescs[i].blockBitSet].push_back(i);
                if (fileDescs[i].unk2[20] != 0)
                    flushQ[8].push_back(i);
            }
        }

        // Final flush
        for (auto& [key, queue] : flushQ) {
            if (!queue.empty()) {
                readOff -= bitsetOffs[key];
                uint32_t temp = 0;
                for (auto idx : queue) {
                    if (key == 8 && fileDescs[idx].blockBitSet != 8) {
                        temp = fileDescs[idx].offset2 + 16;
                        bitsetOffs[key] = temp;
                    } else {
                        absOffsets[idx] = readOff + fileDescs[idx].offset;
                        bitsetOffs[key] = fileDescs[idx].offset + fileDescs[idx].size;
                        temp            = bitsetOffs[key];
                    }
                }
                queue.clear();
                readOff += temp;
            }
        }
    }

    // ── Build semantic tree ───────────────────────────────────────────────
    WadNodeBuilder builder;
    builder.Build(fileDescs, absOffsets, outWad.filename, outWad);

    // Store the decompressed data as the WAD's fileSource so that loaders
    // (SliceFile, etc.) read from the correct decompressed buffer.
    // absOffsets are relative to parsedFile, NOT the original compressed file.
    outWad.fileSource = parsedFile;

    LOG_INFO("[GOWR] Parsed WAD: %u entries → %zu root nodes.",
        header.fileCount, outWad.entries.size());
    return true;
}

bool ProfileGOWR::LoadFromArchive(
    std::shared_ptr<IVirtualFileSystem> vfs, OpenWad& outWad)
{
    // Ragnarök doesn't use ISO archives
    return false;
}

} // namespace GOW
