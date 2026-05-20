#pragma once
#include "core/vfs/IFile.h"
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <atomic>
#include <mutex>

namespace GOW {

struct TexpackEntry {
    uint32_t packIdx;
    uint64_t blockDataOffset;
    uint32_t rawSize;
    uint16_t width;
    uint16_t height;
};

// Lazy + parallel texpack index.
//
// `LoadFromGameRoot` does NOT decompress every `.texpack.toc` upfront. It
// enumerates pack paths, loads the optional `filehashes.csv` patch table,
// and fans out one background TaskManager task per pack. Heavy work (LZ4
// decompression + TOC parsing) runs OUTSIDE the mutex; only the final merge
// into the shared entry map briefly takes the lock. `FindTexture` is a
// non-blocking cache lookup — viewers poll until their hash is published or
// `IsLoaded()` reports completion.
class TexPackIndex {
public:
    void LoadFromGameRoot(const std::filesystem::path& gameRoot);

    // Non-blocking cache lookup. Returns false if the hash hasn't been
    // indexed yet — caller should poll until `IsLoaded()` is true.
    bool FindTexture(uint64_t hash, TexpackEntry& outEntry);

    std::shared_ptr<IFile> GetFile(uint32_t packIdx);

    void SetLoaded() { m_loaded = true; }
    bool IsLoaded() const { return m_loaded; }
    bool IsLoading() const {
        return m_packCount.load() > 0 && m_packsLoaded.load() < m_packCount.load();
    }
    float GetLoadProgress() const {
        int total = m_packCount.load();
        if (total <= 0) return 1.0f;
        return std::max(0.01f, (float)m_packsLoaded.load() / (float)total);
    }

private:
    struct PackInfo {
        std::filesystem::path tocPath;
        std::filesystem::path texpackPath;
        std::shared_ptr<IFile> file;
        bool indexed = false;
    };

    // Decompresses one pack's TOC, parses it, merges entries into the
    // shared map under a brief lock. Safe to call concurrently — workers
    // racing on the same pack are deduplicated by the `indexed` flag.
    bool IndexPack(uint32_t packIdx);

    void LoadFileHashes(const std::filesystem::path& csvLz4Path);

    mutable std::mutex m_mutex;
    std::vector<PackInfo> m_packs;
    std::unordered_map<uint64_t, TexpackEntry> m_entries;
    std::unordered_map<uint64_t, uint32_t> m_hashToPack;

    std::atomic<bool> m_loaded{false};
    std::atomic<int> m_packCount{0};
    std::atomic<int> m_packsLoaded{0};
};

} // namespace GOW
