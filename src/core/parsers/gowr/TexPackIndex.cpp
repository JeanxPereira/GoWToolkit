#include "core/parsers/gowr/TexPackIndex.h"
#include "core/Logger.h"
#include "core/TaskManager.h"
#include "core/vfs/OsFile.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
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

// ── Helpers ────────────────────────────────────────────────────────────────

// Decompress a complete LZ4 frame file into `out`. Returns true on success.
static bool DecompressLz4Frame(const std::filesystem::path& path,
                               std::vector<char>& out)
{
    std::ifstream fs(path, std::ios::in | std::ios::binary);
    if (!fs.is_open()) return false;

    fs.seekg(0, std::ios::end);
    size_t fileSize = (size_t)fs.tellg();
    fs.seekg(0, std::ios::beg);

    std::vector<char> compressed(fileSize);
    fs.read(compressed.data(), fileSize);
    fs.close();

    if (fileSize < 4) return false;
    uint32_t magic = 0;
    memcpy(&magic, compressed.data(), 4);
    if (magic != 0x184D2204) return false;

    LZ4F_dctx* ctx = nullptr;
    if (LZ4F_isError(LZ4F_createDecompressionContext(&ctx, LZ4F_getVersion()))) {
        return false;
    }

    out.assign(4 * 1024 * 1024, 0); // 4 MB initial
    size_t totalDst = 0;
    size_t srcCursor = 0;
    LZ4F_decompressOptions_t opn = { 0, 0, 0, 0 };

    while (true) {
        size_t srcLeft = fileSize - srcCursor;
        size_t dstLeft = out.size() - totalDst;
        if (dstLeft == 0) {
            out.resize(out.size() * 2);
            dstLeft = out.size() - totalDst;
        }
        size_t srcSize = srcLeft;
        size_t dstSize = dstLeft;
        size_t res = LZ4F_decompress(ctx,
                                     out.data() + totalDst, &dstSize,
                                     compressed.data() + srcCursor, &srcSize,
                                     &opn);
        if (LZ4F_isError(res)) {
            LZ4F_freeDecompressionContext(ctx);
            return false;
        }
        totalDst += dstSize;
        srcCursor += srcSize;
        if (res == 0) break;
        if (srcSize == 0 && dstSize == 0) {
            out.resize(out.size() * 2);
        }
    }
    LZ4F_freeDecompressionContext(ctx);
    out.resize(totalDst);
    return true;
}

void TexPackIndex::LoadFileHashes(const std::filesystem::path& csvLz4Path) {
    std::vector<char> buf;
    if (!DecompressLz4Frame(csvLz4Path, buf)) {
        LOG_INFO("[TexPackIndex] filehashes.csv missing or unreadable — patch-hint disabled");
        return;
    }

    m_hashToPack.reserve(16 * 1024);
    const char* p = buf.data();
    const char* end = p + buf.size();
    while (p < end) {
        while (p < end && (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')) ++p;
        if (p >= end) break;
        char* parseEnd = nullptr;
        uint64_t hash = std::strtoull(p, &parseEnd, 10);
        if (!parseEnd || parseEnd == p) {
            while (p < end && *p != '\n') ++p;
            continue;
        }
        if (parseEnd < end && *parseEnd == ',') {
            ++parseEnd;
            char* idxEnd = nullptr;
            uint32_t pk = (uint32_t)std::strtoul(parseEnd, &idxEnd, 10);
            if (idxEnd && idxEnd != parseEnd) {
                m_hashToPack.emplace(hash, pk);
            }
            p = idxEnd ? idxEnd : parseEnd;
        } else {
            p = parseEnd;
        }
        while (p < end && *p != '\n') ++p;
    }
    LOG_INFO("[TexPackIndex] filehashes.csv: %zu hash hints loaded", m_hashToPack.size());
}

// ── LoadFromGameRoot ───────────────────────────────────────────────────────

void TexPackIndex::LoadFromGameRoot(const std::filesystem::path& gameRoot) {
    if (gameRoot.empty()) { SetLoaded(); return; }

    auto pcLeDir = gameRoot / "exec" / "wad" / "pc_le";
    if (!std::filesystem::exists(pcLeDir)) {
        LOG_WARN("[TexPackIndex] pc_le directory not found: %s", pcLeDir.string().c_str());
        SetLoaded();
        return;
    }

    std::vector<std::filesystem::path> tocPaths;
    for (const auto& dirEntry : std::filesystem::directory_iterator(pcLeDir)) {
        auto path = dirEntry.path();
        if (path.extension().string() != ".toc") continue;
        if (path.stem().extension().string() != ".texpack") continue;
        tocPaths.push_back(path);
    }
    std::sort(tocPaths.begin(), tocPaths.end());

    uint32_t packCount = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_packs.clear();
        m_entries.clear();
        m_hashToPack.clear();
        m_packs.reserve(tocPaths.size());
        for (const auto& tp : tocPaths) {
            PackInfo info;
            info.tocPath = tp;
            info.texpackPath = tp.parent_path() / tp.stem();
            if (!std::filesystem::exists(info.texpackPath)) continue;
            m_packs.push_back(std::move(info));
        }
        packCount = (uint32_t)m_packs.size();
        m_packCount = (int)packCount;
        m_packsLoaded = 0;
        LoadFileHashes(pcLeDir / "filehashes.csv");
    }
    m_loaded = false;

    LOG_INFO("[TexPackIndex] Lazy mode: %u packs enumerated, dispatching parallel index", packCount);

    if (packCount == 0) { SetLoaded(); return; }

    // Heuristic: prioritize `root.texpack` first — it holds cross-pack
    // textures referenced by character WADs and is the most common early hit.
    std::vector<uint32_t> order;
    order.reserve(packCount);
    for (uint32_t i = 0; i < packCount; ++i) {
        if (m_packs[i].tocPath.stem().stem().string() == "root") {
            order.insert(order.begin(), i);
        } else {
            order.push_back(i);
        }
    }

    // Fan out — one background task per pack. Heavy work (LZ4 + parse) runs
    // outside the mutex; only the final merge into m_entries holds it briefly.
    for (uint32_t packIdx : order) {
        TaskManager::createBackgroundTask(
            "TexPack: " + m_packs[packIdx].tocPath.stem().stem().string(),
            [this, packIdx]() {
                this->IndexPack(packIdx);
                if (m_packsLoaded.load(std::memory_order_acquire) >= m_packCount.load()) {
                    SetLoaded();
                    LOG_INFO("[TexPackIndex] All packs indexed (background)");
                }
            });
    }
}

// ── IndexPack — heavy work outside lock ────────────────────────────────────

bool TexPackIndex::IndexPack(uint32_t packIdx) {
    std::filesystem::path tocPath;
    std::filesystem::path texpackPath;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (packIdx >= m_packs.size()) return false;
        if (m_packs[packIdx].indexed) return true;
        // Mark eagerly to prevent a parallel worker from racing on the same
        // pack. If parse fails below, we still don't retry.
        m_packs[packIdx].indexed = true;
        tocPath     = m_packs[packIdx].tocPath;
        texpackPath = m_packs[packIdx].texpackPath;
    }

    // No lock held below — parallel workers can run in parallel.
    std::vector<char> buffer;
    if (!DecompressLz4Frame(tocPath, buffer)) {
        LOG_WARN("[TexPackIndex] Failed to decompress %s", tocPath.string().c_str());
        m_packsLoaded.fetch_add(1, std::memory_order_release);
        return false;
    }

    if (buffer.size() < 0x38) {
        m_packsLoaded.fetch_add(1, std::memory_order_release);
        return false;
    }

    uint32_t blocksCount = 0, blocksInfoOff = 0, texsCount = 0;
    memcpy(&blocksCount,  buffer.data() + 0x24, 4);
    memcpy(&blocksInfoOff, buffer.data() + 0x28, 4);
    memcpy(&texsCount,    buffer.data() + 0x2C, 4);

    if (texsCount == 0 || texsCount > 100000 ||
        buffer.size() < 0x38 + texsCount * sizeof(RawTexInfo) ||
        buffer.size() < blocksInfoOff + blocksCount * sizeof(RawBlockInfo))
    {
        m_packsLoaded.fetch_add(1, std::memory_order_release);
        return false;
    }

    std::vector<RawTexInfo> texInfos(texsCount);
    memcpy(texInfos.data(), buffer.data() + 0x38, texsCount * sizeof(RawTexInfo));

    std::unordered_map<uint64_t, RawBlockInfo> blockMap;
    blockMap.reserve(blocksCount);
    const RawBlockInfo* pBlockArr = reinterpret_cast<const RawBlockInfo*>(
        buffer.data() + blocksInfoOff);
    for (uint32_t i = 0; i < blocksCount; i++) {
        uint64_t curOff = blocksInfoOff + i * sizeof(RawBlockInfo);
        blockMap[curOff] = pBlockArr[i];
    }

    // Build per-pack partial map first; merge into shared map at the end.
    std::unordered_map<uint64_t, TexpackEntry> partial;
    partial.reserve(texsCount);
    uint32_t indexed = 0;
    for (const auto& ti : texInfos) {
        uint64_t curBlockOff = ti.blockInfoOff;
        RawBlockInfo rootBlock{};
        bool found = false;
        for (int depth = 0; depth < 16; depth++) {
            auto it = blockMap.find(curBlockOff);
            if (it == blockMap.end()) break;
            rootBlock = it->second;
            if (it->second.nextSiblingBlockInfoOff == 0xFFFFFFFFFFFFFFFFULL) {
                found = true; break;
            }
            curBlockOff = it->second.nextSiblingBlockInfoOff;
        }
        if (!found) {
            auto it = blockMap.find(ti.blockInfoOff);
            if (it != blockMap.end()) { rootBlock = it->second; found = true; }
        }
        if (!found) continue;

        TexpackEntry pEntry;
        pEntry.packIdx = packIdx;
        pEntry.blockDataOffset = (uint64_t)rootBlock.blockOff << 4;
        pEntry.rawSize = rootBlock.rawSize;
        pEntry.width   = rootBlock.mipWidth;
        pEntry.height  = rootBlock.mipHeight;
        partial.emplace(ti.fileHash, pEntry);
        indexed++;
    }

    auto file = std::make_shared<OsFile>(texpackPath.string());

    // Brief lock to publish results.
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_packs[packIdx].file = std::move(file);
        for (auto& kv : partial) {
            m_entries.emplace(kv.first, kv.second);
        }
    }
    m_packsLoaded.fetch_add(1, std::memory_order_release);

    LOG_INFO("[TexPackIndex] Indexed %s: %u/%u",
             tocPath.stem().filename().string().c_str(),
             indexed, texsCount);
    return true;
}

// ── FindTexture — cache lookup only, non-blocking ──────────────────────────

bool TexPackIndex::FindTexture(uint64_t hash, TexpackEntry& outEntry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entries.find(hash);
    if (it == m_entries.end()) return false;
    outEntry = it->second;
    return true;
}

std::shared_ptr<IFile> TexPackIndex::GetFile(uint32_t packIdx) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (packIdx >= m_packs.size()) return nullptr;
    return m_packs[packIdx].file;
}

} // namespace GOW
