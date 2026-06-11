#include "IsoFileSystem.h"
#include "IsoFile.h"
#include "OsFile.h"
#include "Core/Logger.h"
#include <iostream>
#include <vector>

namespace Onyx::Vfs {

IsoFileSystem::IsoFileSystem(const std::string& isoPath) : m_path(isoPath) {
}

// Helper: read PVD at the given layer base, populate root LBA/size, return true on success
static bool ReadPVD(OsFile& file, uint64_t layerBase, uint32_t& outRootLBA, uint32_t& outRootSize, uint32_t& outVolumeSize) {
    file.Seek((int64_t)(layerBase + 16 * 2048), SEEK_SET);

    uint8_t type = 0;
    file.Read(&type, 1);
    char sig[5] = {};
    file.Read(sig, 5);

    if (type != 1 || std::string(sig, 5) != "CD001")
        return false;

    // Volume space size (LE) at offset 80 in PVD
    file.Seek((int64_t)(layerBase + 16 * 2048 + 80), SEEK_SET);
    file.Read(&outVolumeSize, 4);

    // Root directory record at PVD+156; LBA at +2, size at +10
    file.Seek((int64_t)(layerBase + 16 * 2048 + 156 + 2), SEEK_SET);
    file.Read(&outRootLBA, 4);

    file.Seek((int64_t)(layerBase + 16 * 2048 + 156 + 10), SEEK_SET);
    file.Read(&outRootSize, 4);

    return true;
}

bool IsoFileSystem::Initialize() {
    OsFile file(m_path);
    if (!file.IsValid()) return false;

    // â”€â”€ Layer 1 â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    uint32_t rootLBA = 0, rootSize = 0, volumeSize = 0;
    if (!ReadPVD(file, 0, rootLBA, rootSize, volumeSize)) {
        LOG_ERR("Invalid ISO9660 signature.");
        return false;
    }

    ParseDirectoryRecord(rootLBA, rootSize, "/", 0);

    // â”€â”€ Layer 2 (DVD-9 dual-layer) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // The second layer starts at (volumeSize - 16) * 2048 bytes into the file.
    // Reference: god_of_war_browser â€” "Detected second layer of disk. Start: 0xfe618000"
    if (volumeSize > 0) {
        file.Seek(0, SEEK_END);
        int64_t fileSize = file.Tell();

        uint64_t secondLayerBase = ((uint64_t)volumeSize - 16) * 2048;

        // Sanity check: second layer PVD must fit inside the file
        if ((int64_t)(secondLayerBase + 17 * 2048) <= fileSize) {
            uint32_t rootLBA2 = 0, rootSize2 = 0, volSize2 = 0;
            if (ReadPVD(file, secondLayerBase, rootLBA2, rootSize2, volSize2)) {
                LOG_INFO("[ISO] Detected second layer. Base: 0x%llX", (unsigned long long)secondLayerBase);
                ParseDirectoryRecord(rootLBA2, rootSize2, "/", secondLayerBase);
            }
        }
    }

    m_isValid = true;
    return true;
}

bool IsoFileSystem::IsValid() const {
    return m_isValid;
}

void IsoFileSystem::ParseDirectoryRecord(uint32_t sector, uint32_t size, const std::string& currentPath, uint64_t baseOffset) {
    if (size == 0) return;

    OsFile file(m_path);
    if (!file.IsValid()) return;

    file.Seek((int64_t)(baseOffset + (uint64_t)sector * 2048), SEEK_SET);
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
        entry.offset = baseOffset + (uint64_t)extentLba * 2048;
        entry.size = dataSize;
        entry.isDirectory = (flags & 2) != 0;

        // Don't overwrite a layer-1 entry with the same name from layer 2
        // (layer 1 has priority; layer 2 only adds new entries like PART2.PAK)
        if (m_entries.find(fullPath) == m_entries.end())
            m_entries[fullPath] = entry;

        if (entry.isDirectory) {
            ParseDirectoryRecord(extentLba, dataSize, fullPath, baseOffset);
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
    
    auto file = std::make_unique<IsoFile>(m_path, it->second.offset, it->second.size);
    if (!file->IsValid()) return nullptr;
    
    return file;
}

bool IsoFileSystem::Exists(const std::string& path) {
    std::string norm = path;
    if (norm.empty() || norm[0] != '/') norm = "/" + norm;
    return m_entries.find(norm) != m_entries.end();
}

} // namespace Onyx::Vfs
