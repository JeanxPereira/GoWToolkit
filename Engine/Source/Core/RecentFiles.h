#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace Onyx::Services {

/// A single recent file entry with its path and detected game type.
struct RecentEntry {
    std::string path;       // absolute file path
    std::string gameHint;   // "gow1", "gow2", "ragnarok", etc.
    std::string fileType;   // "ISO", "WAD"
    std::string displayName; // e.g. "God of War II (USA).iso"
};

/// Manages a list of recently opened files, persisted as a simple text file.
class RecentFiles {
public:
    static constexpr int MAX_RECENTS = 10;

    /// Load recents from disk
    void Load(const std::string& path);

    /// Save recents to disk
    void Save(const std::string& path) const;

    /// Add a file to the top of the recents list (removes duplicates)
    void Add(const std::string& filePath, const std::string& gameHint, const std::string& fileType);

    /// Get the list of recent entries
    const std::vector<RecentEntry>& Entries() const { return m_entries; }

    /// Check if list is empty
    bool Empty() const { return m_entries.empty(); }

    /// Clear all recents
    void Clear() { m_entries.clear(); }

private:
    std::vector<RecentEntry> m_entries;
    std::string m_savePath;
};

} // namespace Onyx::Services
