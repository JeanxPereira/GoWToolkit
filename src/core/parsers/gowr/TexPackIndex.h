#pragma once
#include "core/vfs/IFile.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <atomic>

namespace GOW {

struct TexpackEntry {
    uint32_t packIdx;
    uint64_t blockDataOffset; // Start of block in .texpack (blockOff << 4)
    uint32_t rawSize;
    uint16_t width;
    uint16_t height;
};

class TexPackIndex {
public:
    void LoadFromGameRoot(const std::filesystem::path& gameRoot);
    bool FindTexture(uint64_t hash, TexpackEntry& outEntry) const;
    std::shared_ptr<IFile> GetFile(uint32_t packIdx) const;
    
    void SetLoaded() { m_loaded = true; }
    bool IsLoaded() const { return m_loaded; }
    
    bool IsLoading() const { return !m_loaded && m_packCount > 0; }
    float GetLoadProgress() const { 
        if (m_packCount > 0) return std::max(0.01f, (float)m_packsLoaded / (float)m_packCount);
        return 0.0f;
    }

private:
    std::unordered_map<uint64_t, TexpackEntry> m_entries;
    std::vector<std::shared_ptr<IFile>> m_packs;
    std::atomic<bool> m_loaded{false};
    std::atomic<int> m_packCount{0};
    std::atomic<int> m_packsLoaded{0};
};

} // namespace GOW
