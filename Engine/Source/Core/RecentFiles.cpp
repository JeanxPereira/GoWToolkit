#include "Core/RecentFiles.h"
#include <fstream>
#include <algorithm>

// File format: one entry per line, tab-separated:
// path<TAB>gameHint<TAB>fileType

void RecentFiles::Load(const std::string& path) {
    m_savePath = path;
    m_entries.clear();

    std::ifstream f(path);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;

        // Parse tab-separated: path \t gameHint \t fileType
        size_t t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        size_t t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;

        RecentEntry entry;
        entry.path     = line.substr(0, t1);
        entry.gameHint = line.substr(t1 + 1, t2 - t1 - 1);
        entry.fileType = line.substr(t2 + 1);

        // Build display name from filename
        fs::path p(entry.path);
        entry.displayName = p.filename().string();

        // Only add if file still exists
        if (fs::exists(p) && m_entries.size() < MAX_RECENTS) {
            m_entries.push_back(entry);
        }
    }
}

void RecentFiles::Save(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return;

    for (const auto& entry : m_entries) {
        f << entry.path << '\t' << entry.gameHint << '\t' << entry.fileType << '\n';
    }
}

void RecentFiles::Add(const std::string& filePath, const std::string& gameHint, const std::string& fileType) {
    // Remove existing entry with the same path
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
            [&](const RecentEntry& e) { return e.path == filePath; }),
        m_entries.end());

    // Build new entry
    RecentEntry entry;
    entry.path        = filePath;
    entry.gameHint    = gameHint;
    entry.fileType    = fileType;
    entry.displayName = fs::path(filePath).filename().string();

    // Insert at the front
    m_entries.insert(m_entries.begin(), entry);

    // Cap at MAX_RECENTS
    if ((int)m_entries.size() > MAX_RECENTS)
        m_entries.resize(MAX_RECENTS);

    // Auto-save
    if (!m_savePath.empty())
        Save(m_savePath);
}
