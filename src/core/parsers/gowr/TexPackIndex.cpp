#include "core/parsers/gowr/TexPackIndex.h"
#include "core/Logger.h"
#include "core/vfs/OsFile.h"
#include <fstream>
#include <lz4frame.h>

namespace GOW {

#pragma pack(push, 1)
struct RawTexInfo {
    uint64_t fileHash;
    uint64_t userHash;
    uint64_t blockInfoOff;
};

struct RawBlockInfo {
    uint32_t blockOff;
    uint32_t rawSize;
    uint64_t blockSize;
    uint8_t  mipLvlStart;
    uint8_t  mipLvlEnd;
    uint16_t tocFileIdx;
    uint16_t mipWidth;
    uint16_t mipHeight;
    uint64_t nextSiblingBlockInfoOff;
};
#pragma pack(pop)

void TexPackIndex::LoadFromGameRoot(const std::filesystem::path& gameRoot) {
    if (gameRoot.empty()) return;

    auto pcLeDir = gameRoot / "exec" / "wad" / "pc_le";
    if (!std::filesystem::exists(pcLeDir)) {
        LOG_WARN("[TexPackIndex] pc_le directory not found: %s", pcLeDir.string().c_str());
        return;
    }

    std::unordered_map<uint64_t, TexpackEntry> localEntries;
    std::vector<std::shared_ptr<IFile>> localPacks;

    std::vector<char> buffer(1024 * 1024 * 50); // Allocate once to avoid zero-init overhead
    
    std::vector<std::filesystem::path> pathsToProcess;
    for (const auto& dirEntry : std::filesystem::directory_iterator(pcLeDir)) {
        auto path = dirEntry.path();
        if (path.extension().string() != ".toc") continue;
        if (path.stem().extension().string() != ".texpack") continue;
        pathsToProcess.push_back(path);
    }
    
    m_packCount = pathsToProcess.size();
    m_packsLoaded = 0;
    
    for (const auto& path : pathsToProcess) {
        
        std::string tocPath = path.string();
        std::string texpackPath = path.parent_path().string() + "/" + path.stem().string();

        if (!std::filesystem::exists(texpackPath)) continue;

        std::ifstream fs(tocPath, std::ios::in | std::ios::binary);
        if (!fs.is_open()) continue;

        fs.seekg(0, std::ios::end);
        size_t fileSize = fs.tellg();
        fs.seekg(0, std::ios::beg);
        
        std::vector<char> compressed(fileSize);
        fs.read(compressed.data(), fileSize);
        fs.close();
        
        uint32_t magic = 0;
        if (fileSize >= 4) memcpy(&magic, compressed.data(), 4);
        
        if (magic != 0x184D2204) continue; // Not LZ4 Frame
        
        // Decompress LZ4 TOC
        LZ4F_dctx* ctx = nullptr;
        LZ4F_createDecompressionContext(&ctx, LZ4F_getVersion());
        LZ4F_decompressOptions_t opn = { 0, 0, 0, 0 };
        
        size_t srcSize = fileSize;
        size_t dstSize = buffer.size();
        size_t res = LZ4F_decompress(ctx, buffer.data(), &dstSize, compressed.data(), &srcSize, &opn);
        LZ4F_freeDecompressionContext(ctx);
        
        if (LZ4F_isError(res)) {
            LOG_WARN("[TexPackIndex] Failed to decompress %s", tocPath.c_str());
            continue;
        }

        if (dstSize < 0x38) continue;
        
        uint32_t texSectionOff = 0, blocksCount = 0, blocksInfoOff = 0, texsCount = 0;
        memcpy(&texSectionOff, buffer.data() + 0x20, 4);
        memcpy(&blocksCount, buffer.data() + 0x24, 4);
        memcpy(&blocksInfoOff, buffer.data() + 0x28, 4);
        memcpy(&texsCount, buffer.data() + 0x2C, 4);
        
        if (texsCount == 0 || texsCount > 100000) continue;
        if (dstSize < 0x38 + texsCount * sizeof(RawTexInfo)) continue;
        
        std::vector<RawTexInfo> texInfos(texsCount);
        memcpy(texInfos.data(), buffer.data() + 0x38, texsCount * sizeof(RawTexInfo));
        
        std::unordered_map<uint64_t, RawBlockInfo> blockMap;
        if (dstSize < blocksInfoOff + blocksCount * sizeof(RawBlockInfo)) continue;
        
        const RawBlockInfo* pBlockArr = reinterpret_cast<const RawBlockInfo*>(buffer.data() + blocksInfoOff);
        for (uint32_t i = 0; i < blocksCount; i++) {
            uint64_t curOff = blocksInfoOff + i * sizeof(RawBlockInfo);
            blockMap[curOff] = pBlockArr[i];
        }

        uint32_t packIdx = static_cast<uint32_t>(localPacks.size());
        auto packFile = std::make_shared<OsFile>(texpackPath);
        localPacks.push_back(packFile);
        
        uint32_t indexed = 0;
        for (const auto& ti : texInfos) {
            uint64_t curBlockOff = ti.blockInfoOff;
            RawBlockInfo rootBlock = {};
            bool found = false;
            
            for (int depth = 0; depth < 16; depth++) {
                auto it = blockMap.find(curBlockOff);
                if (it == blockMap.end()) break;
                
                rootBlock = it->second;
                if (it->second.nextSiblingBlockInfoOff == 0xFFFFFFFFFFFFFFFF) {
                    found = true;
                    break;
                }
                curBlockOff = it->second.nextSiblingBlockInfoOff;
            }
            
            if (!found) {
                auto it = blockMap.find(ti.blockInfoOff);
                if (it != blockMap.end()) {
                    rootBlock = it->second;
                    found = true;
                }
            }
            
            if (found) {
                TexpackEntry pEntry;
                pEntry.packIdx = packIdx;
                pEntry.blockDataOffset = static_cast<uint64_t>(rootBlock.blockOff) << 4;
                pEntry.rawSize = rootBlock.rawSize;
                pEntry.width = rootBlock.mipWidth;
                pEntry.height = rootBlock.mipHeight;
                
                localEntries[ti.fileHash] = pEntry;
                indexed++;
            }
        }

        LOG_INFO("[TexPackIndex] %s: indexed %u textures (of %u)", 
                 path.stem().filename().string().c_str(), indexed, texsCount);
        m_packsLoaded++;
    }
    
    // Atomic swap into member variables — only point where members are mutated
    m_packs = std::move(localPacks);
    m_entries = std::move(localEntries);
    LOG_INFO("[TexPackIndex] Total: %zu textures across %zu packs", m_entries.size(), m_packs.size());
    SetLoaded();
}

std::shared_ptr<IFile> TexPackIndex::GetFile(uint32_t packIdx) const {
    if (packIdx < m_packs.size()) return m_packs[packIdx];
    return nullptr;
}

bool TexPackIndex::FindTexture(uint64_t hash, TexpackEntry& outEntry) const {
    auto it = m_entries.find(hash);
    if (it != m_entries.end()) {
        outEntry = it->second;
        return true;
    }
    return false;
}

} // namespace GOW
