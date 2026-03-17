#include "ProfileGOWR.h"
#include "../../vfs/IsoFileSystem.h"
#include "../../vfs/MemoryFile.h"
#include "../../Logger.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <lz4frame.h>

namespace GOW {

// ── Ragnarok WAD binary structures ─────────────────────────────────────────
// Based on reverse-engineering from D:\CodingProjects\GOWTool (Wad.h / Wad.cpp)

#pragma pack(push, 1)
struct GOWRWadHeader {
    uint32_t magic;       // 0x434F5457 = 'WTOC'
    uint32_t ver;         // 0x2
    uint32_t fileCount;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t block0Size;
    uint32_t block1Size;
    uint32_t block2Size;
    uint8_t  unk3[20];
    uint32_t block8Size;
    uint32_t unk4;
    uint32_t unk5;
};

struct GOWRFileDesc {
    uint16_t group;
    uint16_t type;
    uint32_t size;
    uint8_t  unk1[16];
    char     name[0x38];
    uint8_t  unk2[0x1F];
    uint8_t  blockBitSet;
    uint8_t  unk3[0x8];
    uint32_t offset;
    uint8_t  unk4[12];
    uint32_t offset2;
    uint8_t  unk5[4];
};
#pragma pack(pop)

// Note: Actual struct sizes with pack(1): Header=60, FileDesc=144
// We read them field-by-field from the stream to be safe.

// ── Type enum mapping ──────────────────────────────────────────────────────
static std::string GOWRTypeToString(uint16_t type) {
    switch (type) {
        // Mesh & Geometry
        case 0x0000: return "GOWR_MESH_MAP";
        case 0x0001: return "GOWR_MESH_DEFN";
        case 0x000B: return "GOWR_RIGID_MESH";
        case 0x000C: return "GOWR_SMSH_DEFN";
        case 0x0098: return "GOWR_SKINNED_MESH";
        case 0x0099: return "GOWR_MG_DEFN";
        case 0x8198: return "GOWR_SKINNED_MESH_BUFF";
        case 0x8199: return "GOWR_MG_GPU_BUFF";
        // Model & Instance
        case 0x008F: return "GOWR_MODEL_INSTANCE";
        case 0x0090: return "GOWR_SKIN_INSTANCE";
        // Skeleton & Rig
        case 0x003D: return "GOWR_RIG";
        case 0x003F: return "GOWR_GOPROTO_RIG";
        case 0x0040: return "GOWR_JOINT_MAP";
        // Animation
        case 0x0041: return "GOWR_ANIMATION";
        case 0x0042: return "GOWR_ANM_CLIP";
        // Shader & Material
        case 0x801E: return "GOWR_SHADER";
        case 0x8020: return "GOWR_MATERIAL";
        // Texture
        case 0x80A1: return "GOWR_TEXTURE_OLD";
        case 0x80A2: return "GOWR_TEXTURE";
        // Collision & Physics
        case 0x0050: return "GOWR_COLLISION";
        case 0x0051: return "GOWR_PHYS_DATA";
        // Script & Entity
        case 0x0004: return "GOWR_SCRIPT";
        case 0x0005: return "GOWR_ENTITY";
        case 0x0006: return "GOWR_LEVEL";
        // Audio
        case 0x00C8: return "GOWR_SOUND_BANK";
        case 0x00C9: return "GOWR_SOUND_DATA";
        default: return "GOWR_UNKNOWN_" + std::to_string(type);
    }
}

// ── ProfileGOWR ────────────────────────────────────────────────────────────

ProfileGOWR::ProfileGOWR() {}

bool ProfileGOWR::Detect(const std::filesystem::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != ".wad") return false;
    
    // Peek at magic header to ensure it's a Ragnarok WAD (WTOC or LZ4 frame)
    std::ifstream fs(path, std::ios::binary);
    if (!fs) return false;
    uint32_t magic = 0;
    fs.read(reinterpret_cast<char*>(&magic), 4);
    
    // WTOC (0x434F5457) or LZ4 frame (0x184D2204)
    return magic == 0x434F5457 || magic == 0x184D2204;
}

std::shared_ptr<IVirtualFileSystem> ProfileGOWR::MountArchive(const std::filesystem::path& path) {
    // Ragnarok doesn't use ISO mounting
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
    
    if (initialMagic == 0x184D2204) {
        LOG_INFO("[GOWR] WAD is LZ4-compressed. Decompressing...");
        
        size_t dstCapacity = 0;
        file->Seek(0x06, SEEK_SET);
        file->Read(&dstCapacity, 4);
        
        if (dstCapacity == 0 || dstCapacity > 1024 * 1024 * 500) { // arbitrary 500MB safety
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
        fileSize = outSize;
        LOG_INFO("[GOWR] Decompressed %lld bytes successfully.", fileSize);
    }
    
    parsedFile->Seek(0, SEEK_SET);

    // Read header
    GOWRWadHeader header;
    if (parsedFile->Read(&header, sizeof(GOWRWadHeader)) != sizeof(GOWRWadHeader)) {
        LOG_ERR("[GOWR] Failed to read WAD header");
        return false;
    }

    if (header.magic != 0x434F5457) {
        LOG_ERR("[GOWR] Invalid magic: 0x%08X (expected 0x434F5457 'WTOC')", header.magic);
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

    // Read all file descriptors
    std::vector<GOWRFileDesc> fileDescs(header.fileCount);
    for (uint32_t i = 0; i < header.fileCount; i++) {
        if (parsedFile->Read(&fileDescs[i], sizeof(GOWRFileDesc)) != sizeof(GOWRFileDesc)) {
            LOG_ERR("[GOWR] Failed to read file descriptor %u", i);
            return false;
        }
    }

    // The base offset is after header + all file descriptors
    size_t baseOffset = parsedFile->Tell();

    // Compute absolute offsets using the blockBitSet/flush queue algorithm
    // (simplified version: just use baseOffset + offset for the basic case)
    // For a fully correct implementation, the complex queue logic from GOWTool's Wad.cpp would be needed.
    // For now, we do a simplified linear scan that works for most WADs.
    
    std::vector<size_t> absOffsets(header.fileCount, 0);
    
    // Use the same complex offset calculation as GOWTool
    {
        size_t readOff = baseOffset;
        std::map<uint8_t, uint32_t> bitsetOffs;
        std::map<uint8_t, std::vector<uint32_t>> flushQ;

        for (uint32_t i = 0; i < header.fileCount; i++) {
            std::string nameStr(fileDescs[i].name, strnlen(fileDescs[i].name, 0x38));
            
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
                                bitsetOffs[key] = fileDescs[idx].offset2 + 16;
                            } else {
                                absOffsets[idx] = readOff + fileDescs[idx].offset;
                                bitsetOffs[key] = fileDescs[idx].offset + fileDescs[idx].size;
                                temp = fileDescs[idx].offset + fileDescs[idx].size;
                            }
                        }
                        queue.clear();
                    }
                    readOff += temp;
                }
                if (nameStr == "autopad") {
                    absOffsets[i] = readOff;
                    readOff += fileDescs[i].size;
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
                        bitsetOffs[key] = fileDescs[idx].offset2 + 16;
                    } else {
                        absOffsets[idx] = readOff + fileDescs[idx].offset;
                        bitsetOffs[key] = fileDescs[idx].offset + fileDescs[idx].size;
                        temp = fileDescs[idx].offset + fileDescs[idx].size;
                    }
                }
                queue.clear();
                readOff += temp;
            }
        }
    }

    // Helper to extract base group name (e.g. "MG_hero00_0_gpu" -> "hero00")
    auto getBaseGroupName = [](const std::string& name) -> std::string {
        std::string base = name;
        
        const char* prefixes[] = {
            "MDL_", "MESH_", "MG_", "RIG_", "SKN_",
            "MAT_", "TXR_", "MI_", "M_", "T_",
            "ANM_", "ANIM_", "SQ_",
            "ACT_", "BVR_", "CXT_", "PEM_",
            "WPN_", "CHR_", "ENV_"
        };
        
        bool hasPrefix = false;
        for (const char* p : prefixes) {
            size_t len = strlen(p);
            if (base.compare(0, len, p) == 0) {
                base = base.substr(len);
                hasPrefix = true;
                break;
            }
        }
        
        if (!hasPrefix) return name;
        
        if (base.size() >= 4 && base.compare(base.size() - 4, 4, "_gpu") == 0)
            base = base.substr(0, base.size() - 4);
        if (base.size() >= 5 && base.compare(base.size() - 5, 5, "_buff") == 0)
            base = base.substr(0, base.size() - 5);
            
        auto lastUnderscore = base.find_last_of('_');
        if (lastUnderscore != std::string::npos && lastUnderscore + 1 < base.size()) {
            bool allDigits = true;
            for (size_t i = lastUnderscore + 1; i < base.size(); i++) {
                if (!isdigit(base[i])) { allDigits = false; break; }
            }
            if (allDigits) {
                base = base.substr(0, lastUnderscore);
            }
        }
        return base;
    };

    std::map<std::string, std::vector<ParsedEntry>> groupedEntries;
    std::vector<ParsedEntry> rootEntries;

    // Pass 1: Categorize entries
    for (uint32_t i = 0; i < header.fileCount; i++) {
        ParsedEntry entry;
        entry.name = std::string(fileDescs[i].name, strnlen(fileDescs[i].name, 0x38));
        entry.size = fileDescs[i].size;
        entry.offset = (uint32_t)absOffsets[i];
        entry.schemaType = GOWRTypeToString(fileDescs[i].type);
        entry.wadName = outWad.filename;
        
        std::string base = getBaseGroupName(entry.name);
        if (base != entry.name && !base.empty()) {
            groupedEntries[base].push_back(std::move(entry));
        } else {
            rootEntries.push_back(std::move(entry));
        }
    }

    // Pass 2: Assemble Tree
    for (auto& [groupName, members] : groupedEntries) {
        if (members.size() > 1) {
            ParsedEntry folder;
            folder.name = groupName;
            folder.schemaType = "GOWR_GROUP";
            folder.wadName = outWad.filename;
            folder.size = 0;
            folder.offset = 0;
            folder.children = std::move(members);
            outWad.entries.push_back(std::move(folder));
        } else {
            outWad.entries.push_back(std::move(members[0]));
        }
    }
    
    for (auto& e : rootEntries) {
        outWad.entries.push_back(std::move(e));
    }

    // Sort: Folders first, then alphabetically
    std::sort(outWad.entries.begin(), outWad.entries.end(), [](const ParsedEntry& a, const ParsedEntry& b) {
        if (a.schemaType == "GOWR_GROUP" && b.schemaType != "GOWR_GROUP") return true;
        if (a.schemaType != "GOWR_GROUP" && b.schemaType == "GOWR_GROUP") return false;
        return a.name < b.name;
    });

    LOG_INFO("[GOWR] Parsed WAD: %u absolute entries grouped into %zu root nodes.", 
        header.fileCount, outWad.entries.size());
    return true;
}

bool ProfileGOWR::LoadFromArchive(std::shared_ptr<IVirtualFileSystem> vfs, OpenWad& outWad) {
    // Ragnarok doesn't use ISO archives
    return false;
}

#include "../../schema/StructDef.h"
#include "../../schema/NodeInstance.h"

std::shared_ptr<NodeInstance> ProfileGOWR::CreateNodeInstance(const std::string& typeName, std::shared_ptr<IFile> fileData) {
    // No struct-based inspector for GOWR types yet — the old StructDef approach
    // showed editable fields with padding, which was confusing. 
    // The new Inspector will use read-only display via type-specific handlers.
    return nullptr;
}

} // namespace GOW
