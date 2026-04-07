#include "LodPackIndex.h"
#include "core/Logger.h"
#include <fstream>
#include <cstring>

// ── LodPackIndex.cpp ───────────────────────────────────────────────────────
// See LodPackIndex.h for the binary layout documentation.

namespace GOW {

// ── AddPack ────────────────────────────────────────────────────────────────
int LodPackIndex::AddPack(const std::filesystem::path& path,
                           const std::string& packName)
{
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) {
        LOG_WARN("[LodPackIndex] NOT FOUND: %s", path.string().c_str());
        return 0;
    }

    const int packIdx = static_cast<int>(m_packPaths.size());
    m_packPaths.push_back(path);

    const std::string label = packName.empty() ? path.filename().string() : packName;
    LOG_INFO("[LodPackIndex]   [%d] Opening: %s", packIdx, label.c_str());

    // ── Header ─────────────────────────────────────────────────────────────
    int32_t segCount = 0, lodCount = 0, skip1 = 0, skip2 = 0;
    fs.read(reinterpret_cast<char*>(&segCount), 4);
    fs.read(reinterpret_cast<char*>(&lodCount),  4);
    fs.read(reinterpret_cast<char*>(&skip1),     4);
    fs.read(reinterpret_cast<char*>(&skip2),     4);

    LOG_INFO("[LodPackIndex]     segments=%d  lod_entries=%d", segCount, lodCount);

    if (segCount <= 0 || lodCount <= 0 ||
        segCount > 100000 || lodCount > 5000000) {
        LOG_ERR("[LodPackIndex] Implausible header values — skipping pack");
        return 0;
    }

    // ── Segment base-offset table ─────────────────────────────────────────
    // Each segment entry is 24 bytes: int64 baseOffset + two uint64 skips.
    std::vector<int64_t> segBases(segCount);
    for (int s = 0; s < segCount; ++s) {
        fs.read(reinterpret_cast<char*>(&segBases[s]), 8);  // baseOffset
        uint64_t dummy;
        fs.read(reinterpret_cast<char*>(&dummy), 8);  // skip
        fs.read(reinterpret_cast<char*>(&dummy), 8);  // skip

        if (s < 10) {
            LOG_INFO("[LodPackIndex]     seg[%d] base_offset=0x%llX",
                     s, (unsigned long long)segBases[s]);
        }
    }

    // ── LOD entry table ───────────────────────────────────────────────────
    // Each entry is 20 bytes:
    //   int32  segIndex
    //   int32  relOffset
    //   uint64 hashKey
    //   int32  size
    //   int32  pad
    int added = 0;
    for (int j = 0; j < lodCount; ++j) {
        int32_t  segIdx   = 0;
        int32_t  relOff   = 0;
        uint64_t hashKey  = 0;
        int32_t  size     = 0;
        int32_t  pad      = 0;

        fs.read(reinterpret_cast<char*>(&segIdx),  4);
        fs.read(reinterpret_cast<char*>(&relOff),  4);
        fs.read(reinterpret_cast<char*>(&hashKey), 8);
        fs.read(reinterpret_cast<char*>(&size),    4);
        fs.read(reinterpret_cast<char*>(&pad),     4);

        if (!fs) {
            LOG_WARN("[LodPackIndex] Unexpected EOF at lod[%d]", j);
            break;
        }

        if (segIdx < 0 || segIdx >= segCount) {
            LOG_WARN("[LodPackIndex] lod[%d]: invalid segIdx=%d — skipping", j, segIdx);
            continue;
        }

        LodEntry entry;
        entry.packIdx = packIdx;
        entry.segIdx  = segIdx;
        entry.offset  = segBases[segIdx] + relOff;
        entry.size    = size;

        bool isDup = (m_index.count(hashKey) > 0);
        if (!isDup) {
            m_index[hashKey] = entry;
            ++added;
        }

        if (j < 10 || (j < 20 && !isDup)) {
            LOG_INFO("[LodPackIndex]     lod[%d] key=0x%016llX  seg=%d  off=0x%llX  size=%d  %s",
                     j, (unsigned long long)hashKey, segIdx,
                     (unsigned long long)entry.offset, size,
                     isDup ? "DUPLICATE skipped" : "ADDED");
        }
    }

    LOG_INFO("[LodPackIndex]   Added %d unique entries from %s", added, label.c_str());
    return added;
}

// ── LoadFromList ───────────────────────────────────────────────────────────
int LodPackIndex::LoadFromList(const std::filesystem::path& lodpacksTxtPath,
                                const std::filesystem::path& gameRoot)
{
    std::ifstream txt(lodpacksTxtPath);
    if (!txt.is_open()) {
        LOG_ERR("[LodPackIndex] Cannot open lodpacks.txt: %s",
                lodpacksTxtPath.string().c_str());
        return 0;
    }

    std::vector<std::string> packNames;
    std::string line;
    while (std::getline(txt, line)) {
        // Strip CR/LF
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (!line.empty())
            packNames.push_back(line);
    }

    LOG_INFO("[LodPackIndex] lodpacks.txt: %zu pack(s) listed", packNames.size());

    int total = 0;
    for (size_t i = 0; i < packNames.size(); ++i) {
        auto path = gameRoot / "exec" / "wad" / "pc_le" / (packNames[i] + ".lodpack");
        total += AddPack(path, packNames[i]);
    }

    LOG_INFO("[LodPackIndex] Total unique lod entries indexed: %d", total);
    return total;
}

// ── Find ──────────────────────────────────────────────────────────────────
const LodEntry* LodPackIndex::Find(uint64_t key) const {
    auto it = m_index.find(key);
    if (it != m_index.end())
        return &it->second;

    // C# extractor fallback: if key not found, try (key - 1).
    // Observed in practice for some submeshes with off-by-one hash.
    if (key > 0) {
        it = m_index.find(key - 1);
        if (it != m_index.end())
            return &it->second;
    }

    return nullptr;
}

// ── ReadBlob ──────────────────────────────────────────────────────────────
bool LodPackIndex::ReadBlob(const LodEntry& entry,
                             std::vector<uint8_t>& outData) const
{
    if (entry.packIdx < 0 || entry.packIdx >= static_cast<int>(m_packPaths.size()))
        return false;

    std::ifstream fs(m_packPaths[entry.packIdx], std::ios::binary);
    if (!fs.is_open()) {
        LOG_ERR("[LodPackIndex] Cannot open pack[%d]: %s",
                entry.packIdx, m_packPaths[entry.packIdx].string().c_str());
        return false;
    }

    fs.seekg(entry.offset);
    if (!fs) {
        LOG_ERR("[LodPackIndex] Seek failed to offset 0x%llX in pack[%d]",
                (unsigned long long)entry.offset, entry.packIdx);
        return false;
    }

    outData.resize(entry.size);
    fs.read(reinterpret_cast<char*>(outData.data()), entry.size);

    if (!fs) {
        LOG_ERR("[LodPackIndex] Read failed: requested %d bytes at 0x%llX",
                entry.size, (unsigned long long)entry.offset);
        return false;
    }

    return true;
}

} // namespace GOW
