#include "ProfileGOW2.h"
#include "formats/MDL.h"
#include "../../vfs/IsoFileSystem.h"
#include "core/Logger.h"
#include <iostream>

namespace GOW {

ProfileGOW2::ProfileGOW2() {
    RegisterSchemas();
}

void ProfileGOW2::RegisterSchemas() {
    // Registramos os schemas formatados especificamente para GOW 2
    auto mdlSchema = Formats::GOW2::MDL::CreateSchema();
    m_schemas[mdlSchema->GetName()] = std::move(mdlSchema);
}

bool ProfileGOW2::Detect(const std::filesystem::path& path) const {
    // Estratégia de detecção simples: checa por SYSTEM.CNF referenciando SCUS_974.81 (GOW2)
    // Para simplificar no protótipo: aceita se o nome for de wad de ps2
    auto ext = path.extension().string();
    return (ext == ".iso" || ext == ".wad");
}

std::shared_ptr<IVirtualFileSystem> ProfileGOW2::MountArchive(const std::filesystem::path& path) {
    auto vfs = std::make_shared<IsoFileSystem>(path.string());
    if (vfs->Initialize()) {
        LOG_INFO("[GOW2] Successfully mounted ISO: %s", path.string().c_str());
        return vfs;
    }
    return nullptr;
}

#pragma pack(push, 1)
struct RawWadTag {
    uint16_t tag;
    uint16_t flags;
    uint32_t size;
    char name[24];
};
#pragma pack(pop)

bool ProfileGOW2::ParseWad(std::shared_ptr<IFile> file, OpenWad& outWad) {
    if (!file || !file->IsValid()) return false;

    file->Seek(0, SEEK_END);
    int64_t fileSize = file->Tell();
    file->Seek(0, SEEK_SET);

    LOG_INFO("[GOW2] Parsing WAD of size %lld bytes", fileSize);

    int64_t pos = 0;
    
    // Stack of pointers to vectors of entries. We start with the root vector.
    std::vector<std::vector<ParsedEntry>*> stack;
    stack.push_back(&outWad.entries);

    bool newGroupTag = false;
    int totalTags = 0;

    while (pos < fileSize) {
        RawWadTag rawTag;
        if (file->Read(&rawTag, sizeof(RawWadTag)) != sizeof(RawWadTag)) {
            break; // EOF or error
        }

        pos += sizeof(RawWadTag);

        ParsedEntry entry;
        entry.name = std::string(rawTag.name, strnlen(rawTag.name, 24));
        entry.size = rawTag.size;
        entry.offset = pos;
        
        if (entry.name.find("MAT_") == 0) {
            LOG_INFO("[GOW2Wad] Tag=0x%04X Flags=0x%04X Size=0x%08X (%d) Name=%s Offset=0x%llX", 
                     rawTag.tag, rawTag.flags, rawTag.size, rawTag.size, entry.name.c_str(), entry.offset);
        }
        
        switch (rawTag.tag) {
            case 0:  entry.schemaType = "GOW2_ENTITY_COUNT"; break;
            case 1:  entry.schemaType = "GOW2_SERVER_INSTANCE"; break;
            case 2:  entry.schemaType = "GOW2_GROUP_START"; break;
            case 3:  entry.schemaType = "GOW2_GROUP_END"; break;
            case 21: entry.schemaType = "GOW2_HEADER_START"; break;
            case 19: entry.schemaType = "GOW2_HEADER_POP"; break;
            default: entry.schemaType = "GOW2_UNKNOWN_" + std::to_string(rawTag.tag); break;
        }
        
        if (entry.schemaType == "GOW2_SERVER_INSTANCE") {
            size_t dotPos = entry.name.find_last_of('.');
            if (dotPos != std::string::npos) {
                std::string ext = entry.name.substr(dotPos + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
                entry.schemaType = "GOW2_" + ext;
            } else {
                // Fallback to prefix-based typing for names like MAT_kratos1B
                std::string nameLower = entry.name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                
                if (nameLower.find("mdl_") == 0) entry.schemaType = "GOW2_MDL";
                else if (nameLower.find("mesh_") == 0) entry.schemaType = "GOW2_MESH";
                else if (nameLower.find("mat_") == 0) entry.schemaType = "GOW2_MAT";
                else if (nameLower.find("txr_") == 0) entry.schemaType = "GOW2_TXR";
                else if (nameLower.find("anm_") == 0) entry.schemaType = "GOW2_ANM";
                else if (nameLower.find("scp_") == 0) entry.schemaType = "GOW2_SCP";
                else if (nameLower.find("lgt_") == 0) entry.schemaType = "GOW2_LGT";
                else if (nameLower.find("sfx_") == 0) entry.schemaType = "GOW2_SFX";
                else if (nameLower.find("obj_") == 0) entry.schemaType = "GOW2_OBJ";
            }
        }

        entry.wadName = outWad.filename; 

        if (entry.schemaType == "GOW2_GROUP_START") {
            newGroupTag = true;
        } else if (entry.schemaType == "GOW2_GROUP_END") {
            if (!newGroupTag) {
                if (stack.size() > 1) {
                    stack.pop_back();
                }
            } else {
                newGroupTag = false;
            }
        } else if (entry.schemaType != "GOW2_ENTITY_COUNT" && 
                   entry.schemaType != "GOW2_HEADER_START" && 
                   entry.schemaType != "GOW2_HEADER_POP") {
            // Normal node (SERVER_INSTANCE or TT_*)
            std::vector<ParsedEntry>* currentLevel = stack.back();
            currentLevel->push_back(std::move(entry));
            totalTags++;
            
            if (newGroupTag) {
                newGroupTag = false;
                // The newly added node becomes the new parent
                stack.push_back(&(currentLevel->back().children));
            }
        }

        // Skip payload data (aligned to 16 bytes!)
        if (rawTag.size > 0) {
            pos += rawTag.size;
            pos = ((pos + 15) / 16) * 16;
            file->Seek(pos, SEEK_SET);
        }
    }

    LOG_INFO("[GOW2] Parsed WAD: %d elements built into a tree structure.", totalTags);
    return true;
}

#pragma pack(push, 1)
struct RawTocEntryGOW2 {
    char name[24];
    uint32_t size;
    uint32_t encountersCount;
    uint32_t encountersStart;
};
#pragma pack(pop)

bool ProfileGOW2::LoadFromArchive(std::shared_ptr<IVirtualFileSystem> vfs, OpenWad& outWad) {
    auto tocFile = vfs->OpenFile("/GOW2.TOC");
    if (!tocFile) tocFile = vfs->OpenFile("/GODOFWAR.TOC");
    
    if (!tocFile) {
        LOG_ERR("[GOW2] Could not find GODOFWAR.TOC in the ISO.");
        return false;
    }

    LOG_INFO("[GOW2] Parsing TOC... Size: %zu bytes.", (size_t)tocFile->Size());
    
    uint32_t numFiles;
    if (tocFile->Read(&numFiles, 4) != 4) return false;

    std::vector<RawTocEntryGOW2> rawEntries(numFiles);
    tocFile->Read(rawEntries.data(), numFiles * sizeof(RawTocEntryGOW2));

    uint32_t offsetsStart = 4 + (numFiles * sizeof(RawTocEntryGOW2));
    tocFile->Seek(offsetsStart, SEEK_SET);
    
    outWad.filename = "God of War II (ISO)";
    
    const uint32_t SECTOR_SIZE = 2048;
    const uint32_t DVDDL_SPLITLINE = 10000000;

    for (const auto& raw : rawEntries) {
        if (raw.encountersCount == 0) continue;
        
        // Read just the first encounter for now (ignoring fragmentation)
        uint32_t firstOffsetSector;
        tocFile->Seek(offsetsStart + (raw.encountersStart * 4), SEEK_SET);
        tocFile->Read(&firstOffsetSector, 4);
        
        uint32_t pakIndex = firstOffsetSector / DVDDL_SPLITLINE;
        uint32_t realSector = firstOffsetSector % DVDDL_SPLITLINE;
        
        std::string pakName = "PART" + std::to_string(pakIndex + 1) + ".PAK";
        
        ParsedEntry entry;
        entry.name = std::string(raw.name);
        // Trim null terminators and spaces from name
        entry.name.erase(std::find(entry.name.begin(), entry.name.end(), '\0'), entry.name.end());
        
        entry.size = raw.size;
        entry.offset = realSector * SECTOR_SIZE;
        entry.wadName = pakName; // Store the pak name so EnsureNodeData knows where to load from
        
        // Try to guess basic type from extension
        size_t dotPos = entry.name.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string ext = entry.name.substr(dotPos + 1);
            if (ext == "MDL") entry.schemaType = "GOW2_MDL";
            else if (ext == "TXR") entry.schemaType = "GOW2_TXR";
            else if (ext == "ANM") entry.schemaType = "GOW2_ANM";
            else if (ext == "WAD") entry.schemaType = "GOW2_WAD_FILE";
            else if (ext == "VAG") entry.schemaType = "GOW2_VAG";
            else if (ext == "VPK" || ext == "VP1" || ext == "VP2" || ext == "VP3" || ext == "VP4")
                entry.schemaType = "GOW2_VPK";
            else entry.schemaType = "UNKNOWN";
        } else {
            entry.schemaType = "UNKNOWN";
        }
        
        // Let's generate a basic hash for the UI since we don't have descriptions anymore
        entry.hash = std::hash<std::string>{}(entry.name);
        
        outWad.entries.push_back(std::move(entry));
    }
    
    LOG_INFO("[GOW2] TOC Parsed successfully. Found %zu files.", outWad.entries.size());
    return true;
}

std::shared_ptr<NodeInstance> ProfileGOW2::CreateNodeInstance(const std::string& typeName, std::shared_ptr<IFile> fileData) {
    auto it = m_schemas.find(typeName);
    if (it != m_schemas.end()) {
        auto instance = std::make_shared<NodeInstance>(it->second);
        instance->ReadFromFile(fileData.get());
        return instance;
    }
    return nullptr;
}

} // namespace GOW
