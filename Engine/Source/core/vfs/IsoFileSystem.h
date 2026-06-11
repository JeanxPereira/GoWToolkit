#pragma once
#include "IVirtualFileSystem.h"
#include <map>
#include <string>

namespace Onyx {

struct IsoEntry {
    std::string name;
    uint64_t offset;  // absolute byte offset within the ISO file
    uint32_t size;
    bool isDirectory;
};

class IsoFileSystem : public IVirtualFileSystem {
public:
    IsoFileSystem(const std::string& isoPath);
    ~IsoFileSystem() override = default;

    bool Initialize();

    bool IsValid() const override;
    std::vector<std::string> ListDirectory(const std::string& path) override;
    std::unique_ptr<IFile> OpenFile(const std::string& path) override;
    bool Exists(const std::string& path) override;

    const std::map<std::string, IsoEntry>& GetEntries() const { return m_entries; }
    const std::string& GetPath() const { return m_path; }

private:
    void ParseDirectoryRecord(uint32_t sector, uint32_t size, const std::string& currentPath, uint64_t baseOffset = 0);
    
    
    bool m_isValid = false;
    std::string m_path;
    std::map<std::string, IsoEntry> m_entries;
};

} // namespace Onyx
