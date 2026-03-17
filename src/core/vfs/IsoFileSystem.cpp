#include "IsoFileSystem.h"
#include "IsoFile.h"
#include "OsFile.h"
#include "core/Logger.h"
#include <iostream>
#include <vector>

namespace GOW {

IsoFileSystem::IsoFileSystem(const std::string& isoPath) : m_path(isoPath) {
}

bool IsoFileSystem::Initialize() {
    OsFile file(m_path);
    if (!file.IsValid()) return false;

    // LBA 16 (16 * 2048) contains the Primary Volume Descriptor in ISO 9660
    file.Seek(16 * 2048, SEEK_SET);
    
    uint8_t type;
    file.Read(&type, 1);
    
    char sig[5];
    file.Read(sig, 5);
    
    if (type != 1 || std::string(sig, 5) != "CD001") {
        LOG_ERR("Invalid ISO9660 signature.");
        return false;
    }
    
    // Root directory record is at offset 156 in the PVD (size 34 bytes)
    file.Seek(16 * 2048 + 156, SEEK_SET);
    uint32_t rootExtentLBA;
    // Offset 2 in dir record is Extent LBA (Little Endian then Big Endian, so total 8 bytes. First 4 are LE)
    file.Seek(16 * 2048 + 156 + 2, SEEK_SET); 
    file.Read(&rootExtentLBA, 4);
    
    uint32_t rootDataSize;
    // Offset 10 is Data Length (8 bytes, first 4 are LE)
    file.Seek(16 * 2048 + 156 + 10, SEEK_SET);
    file.Read(&rootDataSize, 4);
    
    ParseDirectoryRecord(rootExtentLBA, rootDataSize, "/");
    m_isValid = true;
    return true;
}

bool IsoFileSystem::IsValid() const {
    return m_isValid;
}

void IsoFileSystem::ParseDirectoryRecord(uint32_t sector, uint32_t size, const std::string& currentPath) {
    if (size == 0) return;
    
    OsFile file(m_path);
    if (!file.IsValid()) return;
    
    file.Seek(sector * 2048, SEEK_SET);
    size_t bytesRead = 0;
    
    while (bytesRead < size) {
        uint8_t len;
        file.Read(&len, 1);
        if (len == 0) {
            bytesRead++;
            // ISO directories might pad sectors with zeros
            continue;
        }
        
        file.Seek(file.Tell() - 1, SEEK_SET); // rewind to start of record
        
        std::vector<uint8_t> rec(len);
        file.Read(rec.data(), len);
        bytesRead += len;
        
        uint32_t extentLba = *reinterpret_cast<uint32_t*>(&rec[2]);
        uint32_t dataSize = *reinterpret_cast<uint32_t*>(&rec[10]);
        uint8_t flags = rec[25];
        uint8_t nameLen = rec[32];
        
        std::string name(reinterpret_cast<char*>(&rec[33]), nameLen);
        
        // Remove ISO 9660 version suffix e.g. ";1"
        size_t semi = name.find(';');
        if (semi != std::string::npos) name = name.substr(0, semi);
        
        if (nameLen == 1 && (rec[33] == '\x00' || rec[33] == '\x01')) continue; // Current/Parent dir
        if (name.empty()) continue;
        
        std::string fullPath = currentPath;
        if (fullPath.back() != '/') fullPath += "/";
        fullPath += name;
        
        IsoEntry entry;
        entry.name = name;
        entry.lba = extentLba;
        entry.size = dataSize;
        entry.isDirectory = (flags & 2) != 0;
        
        m_entries[fullPath] = entry;
        
        if (entry.isDirectory) {
            ParseDirectoryRecord(extentLba, dataSize, fullPath);
        }
    }
}

std::vector<std::string> IsoFileSystem::ListDirectory(const std::string& path) {
    std::vector<std::string> results;
    // Not fully implemented for recursive directory listening yet since OpenFile matters most.
    return results;
}

std::unique_ptr<IFile> IsoFileSystem::OpenFile(const std::string& path) {
    // path must be normalized (e.g., "/SYSTEM.CNF")
    std::string norm = path;
    if (norm.empty() || norm[0] != '/') norm = "/" + norm;
    
    auto it = m_entries.find(norm);
    if (it == m_entries.end() || it->second.isDirectory) return nullptr;
    
    auto file = std::make_unique<IsoFile>(m_path, it->second.lba * 2048, it->second.size);
    if (!file->IsValid()) return nullptr;
    
    return file;
}

bool IsoFileSystem::Exists(const std::string& path) {
    std::string norm = path;
    if (norm.empty() || norm[0] != '/') norm = "/" + norm;
    return m_entries.find(norm) != m_entries.end();
}

} // namespace GOW
