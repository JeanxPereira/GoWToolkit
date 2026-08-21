#include "MgParser.h"
#include <Onyx/Services/Logger.h>
#include <cstring>

// -- MgParser.cpp -------------------------------------------------------------
// MG layout, verified against GoWR.exe and against r_athena00 asset bytes.
// See docs/GoWRknk/Formats/Mesh.md.
//
//   +0x30  u16      PartCount
//   +0x44  u32[PartCount]              offsets to part records, from MG base
//
//   palette range table @ 0x44 + PartCount*4, one (u16 start, u16 count) per
//   part, cumulative. Palette entries follow it, 20 bytes each, with the global
//   bone index as a u16 at +0x00.
//
//   part record:
//     +0x00  u16    BoneRef
//     +0x02  u8     LevelCount        includes the trailing terminator block
//     +0x03  u8     Kind              2 = skinned, 0 = rigid
//     +0x38  u32[LevelCount]          offsets to level blocks, from record base
//
//   level block (16 bytes):
//     +0x00  u32    Kind              0 = draw nothing, 1 = one submesh, 2 = two
//     +0x04  f32    MaxDistance
//     +0x0A  u16    Submesh0
//     +0x0C  u16    Submesh1          only when Kind == 2
//
// The last block of every chain carries MaxDistance = 32767. Its Kind decides
// what happens past the final range: 0 culls the part, >=1 keeps the lowest
// level drawing forever. That block is the terminator only in the Kind == 0
// case; otherwise it is a real level.

namespace Onyx {

namespace {
constexpr float  kInfiniteRange = 32767.0f;
constexpr size_t kPaletteStride = 20;
// Highest submesh count seen in a level block is 10; the bound only
// guards against a malformed file, not against legitimate content.
constexpr uint32_t kMaxLevelSubmeshes = 32;
}

bool GOWRMgParser::Parse(std::shared_ptr<Vfs::IFile> file,
                         uint32_t meshSubmeshCount,
                         Data& out)
{
    out = Data{};
    out.partOfSubmesh.assign(meshSubmeshCount, -1);
    out.levelOfSubmesh.assign(meshSubmeshCount, -1);

    if (!file || !file->IsValid()) return false;
    const size_t fileSize = file->Size();
    if (fileSize < 0x48) return false;

    file->Seek(0x30, SEEK_SET);
    uint16_t partCount = 0;
    file->Read(&partCount, 2);
    if (partCount == 0) return true;

    const size_t ptrTable = 0x44;
    if (fileSize < ptrTable + (size_t)partCount * 4) {
        ONYX_LOGF_WARN("[GOWRMgParser] part table does not fit in %zu bytes", fileSize);
        return false;
    }

    // -- Palette ------------------------------------------------------------
    // Range table sits immediately after the part-offset table; entries follow
    // it. Start values run cumulatively from zero, which is what validates the
    // table before any of it is trusted.
    const size_t rangeTable = ptrTable + (size_t)partCount * 4;
    std::vector<std::pair<uint16_t, uint16_t>> ranges(partCount);
    bool paletteOk = fileSize >= rangeTable + (size_t)partCount * 4;
    uint32_t running = 0;
    if (paletteOk) {
        file->Seek(rangeTable, SEEK_SET);
        for (uint16_t i = 0; i < partCount && paletteOk; ++i) {
            uint16_t start = 0, count = 0;
            file->Read(&start, 2);
            file->Read(&count, 2);
            if (start != running) {
                ONYX_LOGF_WARN("[GOWRMgParser] palette range %u starts at %u, expected %u"
                         " - palette ignored", i, start, running);
                paletteOk = false;
                break;
            }
            running += count;
            ranges[i] = { start, count };
        }
    }
    const size_t paletteBase = rangeTable + (size_t)partCount * 4;
    if (paletteOk && fileSize < paletteBase + (size_t)running * kPaletteStride) {
        ONYX_LOGF_WARN("[GOWRMgParser] palette of %u entries does not fit - ignored", running);
        paletteOk = false;
    }

    // -- Parts --------------------------------------------------------------
    out.parts.resize(partCount);
    int levelsTotal = 0, submeshRefs = 0;

    for (uint16_t i = 0; i < partCount; ++i) {
        file->Seek(ptrTable + (size_t)i * 4, SEEK_SET);
        uint32_t partOff = 0;
        file->Read(&partOff, 4);
        if (partOff + 0x38 > fileSize) {
            ONYX_LOGF_WARN("[GOWRMgParser] part %u offset 0x%X out of range", i, partOff);
            continue;
        }

        Part& part = out.parts[i];
        file->Seek(partOff, SEEK_SET);
        file->Read(&part.boneRef, 2);
        uint8_t levelCount = 0, kind = 0;
        file->Read(&levelCount, 1);
        file->Read(&kind, 1);
        part.rigid = (kind == 0);

        for (uint8_t j = 0; j < levelCount; ++j) {
            file->Seek(partOff + 0x38 + (size_t)j * 4, SEEK_SET);
            uint32_t blockRel = 0;
            file->Read(&blockRel, 4);
            const size_t block = (size_t)partOff + blockRel;
            // The final block of a record is truncated where the next record
            // begins, so only its first 12 bytes are guaranteed readable.
            if (block + 10 > fileSize) break;

            file->Seek(block, SEEK_SET);
            uint32_t blockKind = 0;
            float    dist      = 0.0f;
            uint16_t pad       = 0;
            file->Read(&blockKind, 4);
            file->Read(&dist, 4);
            file->Read(&pad, 2);

            if (blockKind == 0) {
                // Nothing drawn past the previous level.
                if (dist >= kInfiniteRange) part.culledAtRange = true;
                continue;
            }
            if (blockKind > kMaxLevelSubmeshes) {
                ONYX_LOGF_WARN("[GOWRMgParser] part %u level %u: %u submeshes exceeds "
                         "the sane bound", i, j, blockKind);
                continue;
            }

            // Kind is the submesh count, and the indices follow as consecutive
            // u16. Counts of 3, 4, 6 and 10 all occur - r_heroa00 alone has 36
            // such levels across 23 parts - so capping at two silently drops
            // them and those parts lose whole detail levels.
            //
            // The block grows with the count and is padded to eight bytes, but
            // the size never has to be computed: every block's offset comes
            // from the record's pointer array.
            Level lv;
            lv.maxDistance = dist;
            lv.submeshes.reserve(blockKind);
            for (uint32_t s = 0; s < blockKind; ++s) {
                uint16_t sm = 0;
                file->Read(&sm, 2);
                if (sm < meshSubmeshCount) lv.submeshes.push_back(sm);
            }
            if (lv.submeshes.empty()) continue;

            const int levelIdx = (int)part.levels.size();
            for (uint16_t s : lv.submeshes) {
                out.partOfSubmesh[s]  = i;
                out.levelOfSubmesh[s] = levelIdx;
                ++submeshRefs;
            }
            part.levels.push_back(std::move(lv));
            ++levelsTotal;
        }

        if (paletteOk) {
            const auto [start, count] = ranges[i];
            part.palette.reserve(count);
            for (uint16_t k = 0; k < count; ++k) {
                file->Seek(paletteBase + (size_t)(start + k) * kPaletteStride, SEEK_SET);
                uint16_t bone = 0;
                file->Read(&bone, 2);
                part.palette.push_back(bone);
            }
        }
    }

    int unreferenced = 0;
    for (uint32_t s = 0; s < meshSubmeshCount; ++s)
        if (out.partOfSubmesh[s] < 0) ++unreferenced;

    ONYX_LOGF_INFO("[GOWRMgParser] %u parts, %d levels, %d submesh refs, %d unreferenced, "
             "%u palette entries", partCount, levelsTotal, submeshRefs, unreferenced,
             paletteOk ? running : 0u);
    if (unreferenced > 0) {
        ONYX_LOGF_WARN("[GOWRMgParser] %d of %u submeshes are not reached by any level",
                 unreferenced, meshSubmeshCount);
    }
    return true;
}

bool GOWRMgParser::ParseParentBones(std::shared_ptr<Vfs::IFile> file,
                                    uint32_t meshSubmeshCount,
                                    std::vector<uint16_t>& outParentBone)
{
    Data data;
    outParentBone.assign(meshSubmeshCount, 0xFFFF);
    if (!Parse(std::move(file), meshSubmeshCount, data)) return false;

    for (uint32_t s = 0; s < meshSubmeshCount; ++s) {
        const int p = data.partOfSubmesh[s];
        if (p >= 0) outParentBone[s] = data.parts[p].boneRef;
    }
    return true;
}

} // namespace Onyx
