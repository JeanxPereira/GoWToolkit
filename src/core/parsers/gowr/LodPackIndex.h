#pragma once
// ── LodPackIndex.h ─────────────────────────────────────────────────────────
// Parses one or more .lodpack files and builds a hash → LodEntry lookup table.
//
// Direct port of the lodpack indexing logic from GoWRknk.cs (C# extractor),
// confirmed against the real r_heroa00 run log:
//
//   [LOG][LODPACKS] lodpacks.txt: 15 pack(s) listed
//   [LOG][LODPACKS]   [14] Opening: root.lodpack
//   [LOG][LODPACKS]     segments=4862  lod_entries=19259
//   [LOG][LODPACKS]     seg[0] base_offset=0x8D568
//   ...
//   [LOG][LODPACKS]     lod[0] key=0x0003E24D9BF41301  seg=2486  off=0x51D9C3C8  size=69024
//
// Lodpack binary layout (little-endian):
//   Header:
//     +0x00  int32   segmentCount
//     +0x04  int32   lodEntryCount
//     +0x08  int32   (skip)
//     +0x0C  int32   (skip)
//
//   Segment table (segmentCount × 24 bytes):
//     +0x00  int64   baseOffset
//     +0x08  uint64  (skip)
//     +0x10  uint64  (skip)
//
//   LOD entry table (lodEntryCount × 20 bytes):
//     +0x00  int32   segIndex
//     +0x04  int32   relOffset     → absOffset = segBaseOffsets[segIndex] + relOffset
//     +0x08  uint64  hashKey
//     +0x10  int32   size
//     +0x14  int32   (skip/pad)
//
// Usage:
//   LodPackIndex idx;
//   idx.AddPack("root.lodpack");
//   auto* entry = idx.Find(0x624C3D00EB53CF77ULL);

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace GOW {

struct LodEntry {
    int         packIdx;  // which .lodpack file
    int64_t     offset;   // absolute byte offset inside that file
    int32_t     size;     // byte length of the blob
    int32_t     segIdx;   // which segment (for debug)
};

class LodPackIndex {
public:
    // Add a single .lodpack file to the index.
    // Returns the number of unique LOD entries added (duplicates are skipped).
    // packName is used only for log messages.
    int AddPack(const std::filesystem::path& path, const std::string& packName = "");

    // Bulk-load from a lodpacks.txt + game root (mirrors the C# Main logic).
    // lodpacksTxtPath: path to lodpacks.txt (one filename per line, no extension)
    // gameRoot: game root dir; packs are in <gameRoot>/exec/wad/pc_le/<name>.lodpack
    // Returns total unique entries indexed.
    int LoadFromList(const std::filesystem::path& lodpacksTxtPath,
                     const std::filesystem::path& gameRoot);

    // Look up a LOD entry by hash key.
    // The C# extractor also tries (key - 1) as a fallback; we do the same.
    // Returns nullptr if not found.
    const LodEntry* Find(uint64_t key) const;

    // Raw data for a found entry: reads `entry.size` bytes from the pack file.
    // Returns false if the pack file can't be opened or entry is out of bounds.
    bool ReadBlob(const LodEntry& entry, std::vector<uint8_t>& outData) const;

    int TotalEntries() const { return static_cast<int>(m_index.size()); }
    int TotalPacks()   const { return static_cast<int>(m_packPaths.size()); }

private:
    std::unordered_map<uint64_t, LodEntry> m_index;
    std::vector<std::filesystem::path>     m_packPaths;  // indexed by LodEntry::packIdx
};

} // namespace GOW
