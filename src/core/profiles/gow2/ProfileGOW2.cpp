#include "ProfileGOW2.h"
#include "formats/MDL.h"
#include "../../vfs/IsoFileSystem.h"
#include "core/Logger.h"
#include "core/types/TypeRegistry.h"
#include <iostream>
#include <set>
#include <algorithm>
#include <cstring>

namespace GOW {

ProfileGOW2::ProfileGOW2() {
    // Schemas are registered automatically by TypeHandlers.
}

void ProfileGOW2::RegisterSchemas() {
    // Obsolete — handled by individual TypeHandlers and NodeInstance::Parse
}

bool ProfileGOW2::Detect(const std::filesystem::path& path) const {
    std::string ext = path.extension().string();
    for (auto& c : ext) c = (char)tolower((unsigned char)c);
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

// Tag numbers from GOW2 WAD format (matches reference god_of_war_browser/pack/wad/gow2.go)
static constexpr uint16_t WADTAG_ENTITY_COUNT  = 0;
static constexpr uint16_t WADTAG_SERVER_INST   = 1;
static constexpr uint16_t WADTAG_GROUP_START   = 2;
static constexpr uint16_t WADTAG_GROUP_END     = 3;
static constexpr uint16_t WADTAG_HEADER_POP    = 19;
static constexpr uint16_t WADTAG_HEADER_START  = 21;
// Tags 11-16 are TT_* (tweak template) nodes — added as leaves, no group semantics

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

    // Name → (typeId, offset, size) for resolving zero-sized reference entries.
    // A SERVER_INSTANCE with size=0 is a pointer to a previous definition with the same name.
    // When accessed, we redirect to the real definition's data (same as reference GetNodeById).
    struct DefInfo { TypeId typeId; int64_t offset; uint32_t size; };
    std::unordered_map<std::string, DefInfo> nameToDefinition;

    while (pos < fileSize) {
        RawWadTag rawTag;
        if (file->Read(&rawTag, sizeof(RawWadTag)) != sizeof(RawWadTag)) {
            break; // EOF or error
        }

        pos += sizeof(RawWadTag);
        if (rawTag.tag == WADTAG_ENTITY_COUNT) rawTag.size = 0;

        // ── Handle structural tags that affect the stack but don't produce nodes ──
        if (rawTag.tag == WADTAG_GROUP_START) {
            newGroupTag = true;
            if (rawTag.size > 0) {
                pos += rawTag.size;
                pos = ((pos + 15) / 16) * 16;
                file->Seek(pos, SEEK_SET);
            }
            continue;
        }
        if (rawTag.tag == WADTAG_GROUP_END) {
            if (!newGroupTag) {
                if (stack.size() > 1) stack.pop_back();
            } else {
                newGroupTag = false; // empty group
            }
            if (rawTag.size > 0) {
                pos += rawTag.size;
                pos = ((pos + 15) / 16) * 16;
                file->Seek(pos, SEEK_SET);
            }
            continue;
        }
        // Entity count and header pop are metadata — skip silently (reference: default → NOP)
        if (rawTag.tag == WADTAG_ENTITY_COUNT || rawTag.tag == WADTAG_HEADER_POP) {
            if (rawTag.size > 0) {
                pos += rawTag.size;
                pos = ((pos + 15) / 16) * 16;
                file->Seek(pos, SEEK_SET);
            }
            continue;
        }
        // Only SERVER_INSTANCE (1), TT_* (11-16) and HEADER_START (21) reach the tree.
        // Any other unknown structural tag is silently ignored (reference: default → NOP).
        bool addToTree = (rawTag.tag == WADTAG_SERVER_INST) ||
                         (rawTag.tag >= 11 && rawTag.tag <= 16) ||
                         (rawTag.tag == WADTAG_HEADER_START);
        if (!addToTree) {
            if (rawTag.size > 0) {
                pos += rawTag.size;
                pos = ((pos + 15) / 16) * 16;
                file->Seek(pos, SEEK_SET);
            }
            continue;
        }

        ParsedEntry entry;
        entry.name   = std::string(rawTag.name, strnlen(rawTag.name, 24));
        entry.size   = rawTag.size;
        entry.offset = pos;
        entry.wadName = outWad.filename;

        // ── Type Identification ──
        uint8_t payloadMagic[4] = {0};
        size_t payloadSizeAvailable = 0;
        if (rawTag.size >= 4) {
            file->Read(payloadMagic, 4);
            file->Seek(pos, SEEK_SET); // rewind
            payloadSizeAvailable = 4;
        }

        auto* handler = TypeRegistry::Get().ResolveByTag(GameVersion::GOW2, rawTag.tag, payloadMagic, payloadSizeAvailable);
        entry.typeId = handler ? handler->GetId() : TypeId::Unknown;

        // Set schema string for UI display


        // Fallback type resolution for types not yet in TypeRegistry
        if (entry.typeId == TypeId::Unknown) {
            size_t dotPos = entry.name.find_last_of('.');
            if (dotPos != std::string::npos) {
                std::string ext = entry.name.substr(dotPos + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

                if      (ext == "WAD") { entry.typeId = TypeId::WadFile; }
                else if (ext == "VAG") { entry.typeId = TypeId::VagAudio; }
                else if (ext == "VPK" || ext == "VP1") { entry.typeId = TypeId::VpkVideo; }
                else if (ext == "PSS") { entry.typeId = TypeId::PssVideo; }
                else if (ext == "PSW") { entry.typeId = TypeId::PswVideo; }
                else if (ext == "TXT" || ext == "INI" || ext == "CFG" ||
                         ext == "CSV" || ext == "JSON" || ext == "LOG") {
                    entry.typeId = TypeId::TextPlain;
                }
            } else {
                std::string nameLower = entry.name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                if (nameLower.find("pal_") == 0) {
                    entry.typeId = TypeId::PalData;

                } else {
                    if (rawTag.size >= 4) {
                        uint32_t magic;
                        std::memcpy(&magic, payloadMagic, 4);
                        if ((magic & 0x80000000) != 0) {
                            entry.typeId = TypeId::Chunk;

                        } else {

                            LOG_INFO("[ProfileGOW2] Unknown tag: '%s' size=%u magic=0x%08X", entry.name.c_str(), rawTag.size, magic);
                        }
                    } else {

                        LOG_INFO("[ProfileGOW2] Unknown tag: '%s' size=%u (no magic)", entry.name.c_str(), rawTag.size);
                    }
                }
            }
        }

        // ── Zero-sized reference resolution ──
        // A SERVER_INSTANCE with size=0 is a pointer to a previous definition with the same name.
        // Resolve it to the real data so viewers can read it (mirrors reference GetNodeById lazy resolution).
        if (rawTag.tag == WADTAG_SERVER_INST && rawTag.size == 0) {
            auto it = nameToDefinition.find(entry.name);
            if (it != nameToDefinition.end()) {
                entry.typeId  = it->second.typeId;
                entry.offset  = it->second.offset;
                entry.size    = it->second.size;

            }
        } else if (rawTag.size > 0 && !entry.name.empty()) {
            // Cache real definitions for reference resolution above
            nameToDefinition[entry.name] = { entry.typeId, entry.offset, entry.size };
        }

        entry.kind = KindOf(entry.typeId);

        // ── Add node to tree ──
        std::vector<ParsedEntry>* currentLevel = stack.back();
        currentLevel->push_back(std::move(entry));
        totalTags++;

        // Only SERVER_INSTANCE (tag=1) can become the new parent after a GroupStart.
        // HeaderStart (tag=21) and TT_* (tags 11-16) are added as flat siblings without
        // group semantics — matches reference gow2parseTag exactly.
        if (newGroupTag && rawTag.tag == WADTAG_SERVER_INST) {
            newGroupTag = false;
            stack.push_back(&(currentLevel->back().children));
        }

        // Skip payload data (aligned to 16 bytes)
        if (rawTag.size > 0) {
            pos += rawTag.size;
            pos = ((pos + 15) / 16) * 16;
            file->Seek(pos, SEEK_SET);
        }
    }

    // Pass 2: resolve zero-sized and unknown types from forward references
    std::function<void(std::vector<ParsedEntry>&)> resolveUnknowns = [&](std::vector<ParsedEntry>& list) {
        for (auto& n : list) {
            if (n.size == 0 && n.typeId == TypeId::Unknown && !n.name.empty()) {
                auto it = nameToDefinition.find(n.name);
                if (it != nameToDefinition.end()) {
                    n.typeId = it->second.typeId;
                    n.offset = it->second.offset;
                    n.size   = it->second.size;

                    n.kind = KindOf(n.typeId);
                }
            }
            resolveUnknowns(n.children);
        }
    };
    resolveUnknowns(outWad.entries);

    LOG_INFO("[GOW2] Parsed WAD: %d elements built into a tree structure.", totalTags);
    return true;
}

// ── Shared helper ──────────────────────────────────────────────────────────

static void assignSchemaType(ParsedEntry& entry) {
    size_t dot = entry.name.find_last_of('.');
    if (dot != std::string::npos) {
        std::string ext = entry.name.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

        if      (ext == "MDL") { entry.typeId = GOW::TypeId::Model;    }
        else if (ext == "TXR") { entry.typeId = GOW::TypeId::Texture;  }
        else if (ext == "ANM") { entry.typeId = GOW::TypeId::Animation; }
        else if (ext == "WAD") { entry.typeId = GOW::TypeId::WadFile;  }
        else if (ext == "VAG") { entry.typeId = GOW::TypeId::VagAudio; }
        else if (ext == "VPK" || ext == "VP1" || ext == "VP2" ||
                 ext == "VP3" || ext == "VP4")
                              { entry.typeId = GOW::TypeId::VpkVideo; }
        else if (ext == "PSS") { entry.typeId = GOW::TypeId::PssVideo; }
        else if (ext == "PSW") { entry.typeId = GOW::TypeId::PswVideo; }
        else if (ext == "TXT" || ext == "INI" || ext == "CFG" ||
                 ext == "CSV" || ext == "JSON" || ext == "LOG")
                              { entry.typeId = GOW::TypeId::TextPlain; }

        if (entry.typeId == GOW::TypeId::Unknown) {
            // Unhandled extension
        }
    }
    entry.kind = KindOf(entry.typeId);
}

// ── GOW2 TOC parser ────────────────────────────────────────────────────────
// Format: uint32 fileCount, then fileCount×36-byte entries, then offset array
//   [0:24]  filename (null-padded)
//   [24:28] file size in bytes
//   [28:32] encounter count (number of copies/locations)
//   [32:36] encounter start index into the offset array
// Each offset value:
//   if >= 10000000 → dual-layer disc; pakIndex = value / 10M, sector = value % 10M
//   else           → single-layer rip; all data in PART1.PAK at that sector
#pragma pack(push, 1)
struct RawTocEntryGOW2 {
    char     name[24];
    uint32_t size;
    uint32_t encountersCount;
    uint32_t encountersStart;
};
#pragma pack(pop)

bool ProfileGOW2::LoadFromArchiveGOW2(std::shared_ptr<IVirtualFileSystem> vfs,
                                        IFile* tocFile, OpenWad& outWad) {
    LOG_INFO("[GOW2] Parsing TOC... size: %zu bytes.", (size_t)tocFile->Size());

    uint32_t numFiles = 0;
    if (tocFile->Read(&numFiles, 4) != 4) return false;

    std::vector<RawTocEntryGOW2> rawEntries(numFiles);
    tocFile->Read(rawEntries.data(), numFiles * sizeof(RawTocEntryGOW2));

    const uint32_t offsetsStart = 4 + numFiles * (uint32_t)sizeof(RawTocEntryGOW2);

    outWad.filename = "God of War II (ISO)";

    auto pakExists = [&](const std::string& name) -> bool {
        return vfs->Exists("/" + name) || vfs->Exists(name);
    };
    std::set<std::string> warnedMissingPaks;

    const uint32_t SECTOR_SIZE    = 2048;
    const uint32_t DVDDL_SPLITLINE = 10000000;

    for (const auto& raw : rawEntries) {
        if (raw.encountersCount == 0) continue;

        uint32_t rawSector = 0;
        tocFile->Seek(offsetsStart + raw.encountersStart * 4, SEEK_SET);
        tocFile->Read(&rawSector, 4);

        uint32_t pakIndex  = rawSector / DVDDL_SPLITLINE;
        uint32_t realSector = rawSector % DVDDL_SPLITLINE;
        std::string pakName = "PART" + std::to_string(pakIndex + 1) + ".PAK";

        if (!pakExists(pakName)) {
            if (warnedMissingPaks.insert(pakName).second)
                LOG_WARN("[GOW2] '%s' not found in ISO — skipping its entries. "
                         "(Dual-layer ISOs may need both layers merged.)", pakName.c_str());
            continue;
        }

        ParsedEntry entry;
        entry.name = std::string(raw.name, strnlen(raw.name, 24));
        entry.size   = raw.size;
        entry.offset = (int64_t)realSector * SECTOR_SIZE;
        entry.wadName = pakName;
        entry.hash = std::hash<std::string>{}(entry.name);
        assignSchemaType(entry);
        outWad.entries.push_back(std::move(entry));
    }

    LOG_INFO("[GOW2] TOC parsed: %zu files.", outWad.entries.size());
    return !outWad.entries.empty();
}

// ── LoadFromArchive ───────────────────────────────────────────────────────
bool ProfileGOW2::LoadFromArchive(std::shared_ptr<IVirtualFileSystem> vfs, OpenWad& outWad) {
    // Try GOW2.TOC first (some builds use this name)
    auto tocFile = vfs->OpenFile("/GOW2.TOC");

    // Fall back to GODOFWAR.TOC (default GOW2 layout)
    if (!tocFile || !tocFile->IsValid())
        tocFile = vfs->OpenFile("/GODOFWAR.TOC");

    if (!tocFile || !tocFile->IsValid()) {
        LOG_ERR("[GOW] No TOC file found in ISO (tried GOW2.TOC, GODOFWAR.TOC).");
        return false;
    }

    // Sanity-check GOW2 header: first 4 bytes = file count (small integer),
    // and count * sizeof(RawTocEntryGOW2) + 4 must fit within the file.
    tocFile->Seek(0, SEEK_END);
    int64_t tocSize = tocFile->Tell();
    tocFile->Seek(0, SEEK_SET);

    uint32_t possibleCount = 0;
    tocFile->Read(&possibleCount, 4);
    tocFile->Seek(0, SEEK_SET);

    bool isGOW2 = (possibleCount > 0 && possibleCount < 200000) &&
                  ((int64_t)(possibleCount * sizeof(RawTocEntryGOW2) + 4) <= tocSize);

    if (!isGOW2) {
        LOG_ERR("[GOW] TOC header does not look like GOW2 (count=%u, size=%lld). "
                "GOW1 ISOs are not supported by this profile.",
                possibleCount, (long long)tocSize);
        return false;
    }

    LOG_INFO("[GOW] Detected GOW2 TOC format (%u entries).", possibleCount);
    return LoadFromArchiveGOW2(vfs, tocFile.get(), outWad);
}

} // namespace GOW
