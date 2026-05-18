#include "MgParser.h"
#include "core/Logger.h"

// ── MgParser.cpp ───────────────────────────────────────────────────────────
// MG-file layout (port of GOWTool Formats.cpp::MG::Parse):
//
//   +0x30  uint16  defCount         (number of mg-defs / "bone groups")
//   +0x44  uint32[defCount]         relative-offset table → mg-def
//
//   per mg-def @ defOff:
//     +0x00   uint16   parentBone
//     +0x02   uint8    lodCount
//     +0x38   uint32[lodCount]      relative-offset table → skin list
//
//     per skin list @ skinOff (relative to defOff):
//       +0x00   uint32   submeshCount
//       +0x04   6 bytes  pad
//       then    uint16 × submeshCount   MESH-submesh indices
//
// Vertices of meshes[idx] rigidly belong to parentBone, with LODlvl = j.

namespace GOW {

bool GOWRMgParser::Parse(std::shared_ptr<IFile> file,
                         uint32_t meshSubmeshCount,
                         std::vector<uint16_t>& outParentBone) {
    outParentBone.assign(meshSubmeshCount, 0xFFFF);

    if (!file || !file->IsValid()) return false;
    if (file->Size() < 0x44) return false;

    file->Seek(0x30, SEEK_SET);
    uint16_t defCount = 0;
    file->Read(&defCount, 2);
    if (defCount == 0) return true;
    if (file->Size() < (size_t)(0x44 + defCount * 4)) return false;

    int totalAssigned = 0;

    for (uint16_t i = 0; i < defCount; ++i) {
        file->Seek(0x44 + i * 4, SEEK_SET);
        uint32_t defOff = 0;
        file->Read(&defOff, 4);

        // ── DIAGNOSTIC: dump the 56 bytes at the head of each mg-def so we
        // can manually inspect for MAT hashes / hidden flags. Remove once
        // the mesh→MAT link is identified. (Layout to date: +0x00 parentBone
        // u16, +0x02 lodCount u8, then 53 unknown bytes before +0x38.)
        {
            uint8_t hdr[0x40] = {};
            file->Seek(defOff, SEEK_SET);
            file->Read(hdr, sizeof(hdr));
            char hex[0x40 * 3 + 1] = {};
            for (size_t b = 0; b < sizeof(hdr); ++b) {
                std::snprintf(hex + b * 3, 4, "%02X ", hdr[b]);
            }
            LOG_INFO("[GOWRMgParser] mg-def[%u] @0x%X bytes[0x00..0x3F]: %s",
                     i, defOff, hex);
        }

        file->Seek(defOff, SEEK_SET);
        uint16_t parentBone = 0;
        file->Read(&parentBone, 2);

        uint8_t lodCount = 0;
        file->Read(&lodCount, 1);

        for (uint8_t j = 0; j < lodCount; ++j) {
            file->Seek(defOff + 0x38 + j * 4, SEEK_SET);
            uint32_t skinOff = 0;
            file->Read(&skinOff, 4);

            file->Seek(defOff + skinOff, SEEK_SET);
            uint32_t cnt = 0;
            file->Read(&cnt, 4);
            file->Seek(6, SEEK_CUR); // padding

            for (uint32_t k = 0; k < cnt; ++k) {
                uint16_t idx = 0;
                file->Read(&idx, 2);
                if (idx < meshSubmeshCount && outParentBone[idx] == 0xFFFF) {
                    outParentBone[idx] = parentBone;
                    ++totalAssigned;
                }
            }
        }
    }

    LOG_INFO("[GOWRMgParser] %u mg-defs, %d of %u MESH submeshes assigned a parentBone",
             defCount, totalAssigned, meshSubmeshCount);
    return true;
}

} // namespace GOW
