#include "MeshParser.h"
#include "LodPackIndex.h"
#include <Onyx/Services/Logger.h>
#include <Onyx/Vfs/MemoryFile.h>
#include <cstring>
#include <algorithm>

// â”€â”€ MeshParser.cpp â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Full port of GoWRknk.cs mesh reading logic.
//
// Submesh field map (all offsets relative to submeshBase in MESH file):
//
//  +0x00  uint16  type/flags
//  +0x10  vec3    extent   (scale XYZ, 12 bytes)
//  +0x1C  vec3    origin   (bias  XYZ, 12 bytes)
//  +0x30  uint32  gpuIndexOffset  (abs. offset in GPU file)
//  +0x44  uint32  vertCount
//  +0x48  uint32  faceCount
//  +0x5C  uint32  indCount        (= faceCount * 3)
//  +0x60  uint32  componentOffset (relative to submeshBase)
//  +0x64  uint32  bufOffsetsOffset (relative to submeshBase)
//  +0x68  uint64  meshHash
//  +0x80  uint8   bufferCount
//  +0x81  uint8   indicesStride   (2 = uint16, 4 = uint32)
//  +0x82  uint8   bytesPerVertex  (interleaved stride; valid if bufferCount==1)
//  +0x83  uint8   componentCount
//
//  Component descriptor (8 bytes each, at componentOffset):
//  +0  uint8   semantic
//  +1  uint8   format
//  +2  uint8   compCount
//  +3  uint8   byteOffset
//  +4  uint32  bufferIdx
//
//  Buffer offsets table: uint32 each, at bufOffsetsOffset.
//  Each value is an ABSOLUTE offset inside the GPU/LOD blob.

namespace Onyx {

// â”€â”€ ElementSize â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Bytes per element, keyed by the raw format enum. Transcribed from the
// game's own vertex-view builder, which switches on the format byte:
//   {0,2,3} -> 4    {1,4,5,6,7} -> 2    {8,9,10,11} -> 1
// A component's total size is always elemBytes * compCount.
static uint32_t ElementSize(GOWRMeshParser::AttrFormat fmt, uint8_t compCount) {
    uint32_t elem;
    switch (static_cast<uint8_t>(fmt)) {
        case 0: case 2: case 3:                   elem = 4; break;
        case 1: case 4: case 5: case 6: case 7:   elem = 2; break;
        case 8: case 9: case 10: case 11:         elem = 1; break;
        default:                                  elem = 4; break;
    }
    return elem * compCount;
}

// -- ReadSubmeshTable ---------------------------------------------------------
// Resolves a MESH file's submesh offset table, mirroring the game's own walker.
//
// The game does:  table = (M + 0x0C) + int32_at(M + 0x0C)
//                 count = uint32_at(M + 0x10)
// and each table slot is itself self-relative: submesh = &slot + int32_at(slot).
//
// The table is NOT pinned at 0x40 -- that is just where the common rel value of
// 0x34 lands it. Hardcoding 0x40 walks straight into padding on any file that
// places it elsewhere, which is how a zero region used to be mistaken for tens
// of thousands of submeshes.
static std::vector<uint32_t> ReadSubmeshTable(std::shared_ptr<Vfs::IFile>& f) {
    std::vector<uint32_t> out;
    if (!f || !f->IsValid()) return out;

    const uint64_t fileSize = f->Size();
    if (fileSize < 0x14) return out;

    int32_t  tableRel = 0;
    uint32_t count    = 0;
    f->Seek(0x0C, SEEK_SET);
    f->Read(&tableRel, 4);
    f->Read(&count,    4);   // +0x10, u32

    const int64_t tableAt = static_cast<int64_t>(0x0C) + tableRel;
    if (count == 0) return out;
    if (tableAt < 0x14 ||
        static_cast<uint64_t>(tableAt) + 4ull * count > fileSize) {
        LOG_WARN("[GOWRMeshParser] submesh table out of range: rel=%d -> 0x%llX, "
                 "count=%u, file=0x%llX", tableRel,
                 (unsigned long long)tableAt, count,
                 (unsigned long long)fileSize);
        return out;
    }

    out.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t slot = static_cast<uint32_t>(tableAt) + i * 4;
        int32_t rel = 0;
        f->Seek(slot, SEEK_SET);
        if (f->Read(&rel, 4) != 4) break;

        const int64_t abs = static_cast<int64_t>(slot) + rel;
        // A submesh header runs to +0x88, so anything that cannot hold one is
        // padding rather than an entry.
        if (rel == 0 || abs < 0x14 ||
            static_cast<uint64_t>(abs) + 0x88 > fileSize) {
            LOG_WARN("[GOWRMeshParser] submesh table ends at %u of %u entries",
                     i, count);
            break;
        }
        out.push_back(static_cast<uint32_t>(abs));
    }
    return out;
}

// â”€â”€ ReadSubmeshHeader â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool GOWRMeshParser::ReadSubmeshHeader(std::shared_ptr<Vfs::IFile>& f,
                                       uint32_t base,
                                       SubmeshHeader& h)
{
    // â”€â”€ DIAGNOSTIC: dump first 0x90 bytes of submesh header so we can hunt
    // for MAT hash field placement. Remove once meshâ†’MAT link is identified.
    {
        uint8_t hdr[0x90] = {};
        f->Seek(base, SEEK_SET);
        f->Read(hdr, sizeof(hdr));
        char hex[0x90 * 3 + 1] = {};
        for (size_t b = 0; b < sizeof(hdr); ++b) {
            std::snprintf(hex + b * 3, 4, "%02X ", hdr[b]);
        }
        LOG_DEBUG("[GOWRMeshParser] submesh @0x%X bytes[0x00..0x8F]: %s", base, hex);
    }


    f->Seek(base + 0x10, SEEK_SET);
    f->Read(&h.extent, 12);

    f->Seek(base + 0x1C, SEEK_SET);
    f->Read(&h.origin, 12);

    f->Seek(base + 0x28, SEEK_SET);
    f->Read(&h.materialIndex, 4);

    f->Seek(base + 0x30, SEEK_SET);
    f->Read(&h.gpuIndexOffset, 4);

    f->Seek(base + 0x44, SEEK_SET);
    f->Read(&h.vertCount, 4);
    f->Read(&h.faceCount, 4);

    f->Seek(base + 0x5C, SEEK_SET);
    f->Read(&h.indCount, 4);

    uint32_t compOffRel, bufOffRel;
    f->Read(&compOffRel, 4);   // +0x60
    f->Read(&bufOffRel,  4);   // +0x64
    f->Read(&h.meshHash, 8);   // +0x68

    // +0x80 u8 bufferCount, +0x81 u8 indicesStride, +0x82 u8 bytesPerVertex,
    // +0x83 u8 topology, +0x84 u8 componentCount.
    //
    // +0x82 and +0x83 are two distinct bytes, not one u16: the game reads
    // +0x83 on its own as a primitive-topology enum (masked with 7) and
    // never reads +0x82 at all -- it derives every stride from the
    // component table instead. Reading the pair as a u16 stride yields
    // (topology << 8) | bytesPerVertex, which shreds the geometry of any
    // submesh whose topology is non-zero.
    f->Seek(base + 0x80, SEEK_SET);
    f->Read(&h.bufferCount,    1);
    f->Read(&h.indicesStride,  1);
    f->Read(&h.bytesPerVertex, 1);
    f->Read(&h.topology,       1);
    f->Read(&h.componentCount, 1);

    h.componentOffsetAbs = base + compOffRel;
    h.bufOffsetsAbs      = base + bufOffRel;

    return true;
}

// â”€â”€ ReadComponents â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool GOWRMeshParser::ReadComponents(std::shared_ptr<Vfs::IFile>& f,
                                     const SubmeshHeader& hdr,
                                     std::vector<ComponentDesc>& out)
{
    out.resize(hdr.componentCount);
    f->Seek(hdr.componentOffsetAbs, SEEK_SET);
    for (auto& c : out) {
        // 8 bytes per component: semantic, format, count, byteOffset,
        // bufferIdx, then 3 bytes the game never reads.
        uint8_t s, fmt, cnt, off, bufIdx, pad[3];
        f->Read(&s,      1);
        f->Read(&fmt,    1);
        f->Read(&cnt,    1);
        f->Read(&off,    1);
        f->Read(&bufIdx, 1);
        f->Read(pad,     3);
        c.semantic   = static_cast<Semantic>(s);
        c.format     = static_cast<AttrFormat>(fmt);
        c.compCount  = cnt;
        c.byteOffset = off;
        c.bufferIdx  = bufIdx;
    }
    return true;
}

// â”€â”€ ReadBufferOffsets â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool GOWRMeshParser::ReadBufferOffsets(std::shared_ptr<Vfs::IFile>& f,
                                        const SubmeshHeader& hdr,
                                        std::vector<uint32_t>& out)
{
    out.resize(hdr.bufferCount);
    f->Seek(hdr.bufOffsetsAbs, SEEK_SET);
    for (auto& o : out)
        f->Read(&o, 4);
    return true;
}

// â”€â”€ ReadVertices â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool GOWRMeshParser::ReadVertices(std::shared_ptr<Vfs::IFile>& gpu,
                                   const SubmeshHeader& hdr,
                                   const std::vector<ComponentDesc>& comps,
                                   const std::vector<uint32_t>& bufOffsets,
                                   Parsers::MeshPart& part)
{
    const uint32_t N = hdr.vertCount;
    part.vertices.resize(N);

    // Detect rigid: submesh has no BoneIdx (semantic 9) / BoneWgt (semantic 10).
    // Rigid verts are authored in joint-local space â€” bind them to local-palette
    // slot 0 with full weight so the shader transforms by jointMap[0] only.
    bool hasBoneSemantic = false;
    for (const auto& c : comps) {
        if (c.semantic == Semantic::BoneIdx || c.semantic == Semantic::BoneWgt) {
            hasBoneSemantic = true;
            break;
        }
    }
    part.isRigid = !hasBoneSemantic;

    const glm::vec4  defaultW = part.isRigid ? glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)
                                             : glm::vec4(0.0f);
    const glm::uvec4 defaultI = glm::uvec4(0u);
    for (auto& v : part.vertices) {
        v.boneWeights = defaultW;
        v.boneIndices = defaultI;
    }

    // â”€â”€ Per-buffer strides, derived exactly as the game derives them â”€â”€â”€â”€â”€â”€â”€â”€
    // Every buffer is interleaved: its stride is the sum of the sizes of the
    // components that name it. Treating each component as its own tightly
    // packed stream is only correct when a buffer holds a single component.
    //
    // The buffer-offsets table is *packed*: the game walks logical buffers
    // 0..14 in order and consumes one offset per buffer that has a non-zero
    // stride, so a logical index is not an index into that table.
    constexpr uint32_t kMaxBuffers = 15;
    uint32_t strideOf[kMaxBuffers] = {};
    for (const auto& c : comps) {
        if (c.bufferIdx >= kMaxBuffers) continue;
        strideOf[c.bufferIdx] += ElementSize(c.format, c.compCount);
    }

    int slotOf[kMaxBuffers];
    uint32_t nextSlot = 0;
    for (uint32_t b = 0; b < kMaxBuffers; ++b)
        slotOf[b] = strideOf[b] ? static_cast<int>(nextSlot++) : -1;

    if (nextSlot != bufOffsets.size()) {
        LOG_WARN("[GOWRMeshParser] %u buffers carry components but the offset "
                 "table holds %zu entries", nextSlot, bufOffsets.size());
    }

    for (const auto& c : comps) {
        if (c.bufferIdx >= kMaxBuffers) continue;
        const int slot = slotOf[c.bufferIdx];
        if (slot < 0 || static_cast<size_t>(slot) >= bufOffsets.size()) continue;

        const uint32_t streamBase = bufOffsets[slot];
        const uint32_t stride     = strideOf[c.bufferIdx];

        for (uint32_t vi = 0; vi < N; ++vi) {
            const uint32_t vertOff = streamBase + vi * stride + c.byteOffset;
            gpu->Seek(vertOff, SEEK_SET);

            Domain::GpuVertex& v = part.vertices[vi];

            switch (c.semantic) {

            // â”€â”€ Position â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            case Semantic::Position:
                if (c.format == AttrFormat::Float32) {
                    gpu->Read(&v.position.x, 4);
                    gpu->Read(&v.position.y, 4);
                    gpu->Read(&v.position.z, 4);
                } else if (c.format == AttrFormat::Uint16) {
                    // Quantised: (raw/32768 - 1) * extent + origin
                    uint16_t rx, ry, rz, rw;
                    gpu->Read(&rx, 2);
                    gpu->Read(&ry, 2);
                    gpu->Read(&rz, 2);
                    gpu->Read(&rw, 2);  // padding word, discard
                    v.position.x = ((float)rx / 32768.0f - 1.0f) * hdr.extent.x + hdr.origin.x;
                    v.position.y = ((float)ry / 32768.0f - 1.0f) * hdr.extent.y + hdr.origin.y;
                    v.position.z = ((float)rz / 32768.0f - 1.0f) * hdr.extent.z + hdr.origin.z;
                } else {
                    LOG_WARN("[GOWRMeshParser] Unknown position format %u", (uint8_t)c.format);
                }
                break;

            // â”€â”€ Normal â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            case Semantic::Normal:
                if (c.format == AttrFormat::R10G10B10) {
                    // 3 Ã— 10-bit biased-unsigned packed in uint32 (DirectX style).
                    // Port of GoWRknk.cs:839-843:
                    //   num4 = (float)(num59 & 0x3FF) - 512f) / 512f
                    // Encoding: X=[9:0], Y=[19:10], Z=[29:20], W=[31:30] ignored
                    uint32_t packed;
                    gpu->Read(&packed, 4);
                    auto unpack10 = [](uint32_t p, int shift) -> float {
                        uint32_t u = (p >> shift) & 0x3FF;
                        return ((float)u - 512.0f) / 512.0f;
                    };
                    v.normal.x = unpack10(packed,  0);
                    v.normal.y = unpack10(packed, 10);
                    v.normal.z = unpack10(packed, 20);
                    v.normal   = glm::normalize(v.normal);
                }
                break;

            // â”€â”€ UV0 (primary UV, stream UV1 in C# naming) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            // Port of GoWRknk.cs:865-879:
            //   fmt=0 (float32): raw
            //   fmt=6 (unorm16): raw/65536
            //   fmt=7 (snorm16): raw/32768
            case Semantic::UV0:
                if (c.format == AttrFormat::Float32 && c.compCount >= 2) {
                    gpu->Read(&v.uv.x, 4);
                    gpu->Read(&v.uv.y, 4);
                } else if (c.format == AttrFormat::Uint16 && c.compCount >= 2) {
                    uint16_t u, vv;
                    gpu->Read(&u,  2);
                    gpu->Read(&vv, 2);
                    v.uv.x = (float)u  / 65536.0f;
                    v.uv.y = (float)vv / 65536.0f;
                } else if (c.format == AttrFormat::Int16 && c.compCount >= 2) {
                    int16_t u, vv;
                    gpu->Read(&u,  2);
                    gpu->Read(&vv, 2);
                    v.uv.x = (float)u  / 32768.0f;
                    v.uv.y = (float)vv / 32768.0f;
                }
                break;

            // â”€â”€ UV1 / UV2 / UV3 (lightmap, detail, etc.) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            case Semantic::UV1:
                if (c.format == AttrFormat::Float32 && c.compCount >= 2) {
                    gpu->Read(&v.uv1.x, 4);
                    gpu->Read(&v.uv1.y, 4);
                } else if (c.format == AttrFormat::Uint16 && c.compCount >= 2) {
                    uint16_t u, vv;
                    gpu->Read(&u,  2);
                    gpu->Read(&vv, 2);
                    v.uv1.x = (float)u  / 65536.0f;
                    v.uv1.y = (float)vv / 65536.0f;
                } else if (c.format == AttrFormat::Int16 && c.compCount >= 2) {
                    int16_t u, vv;
                    gpu->Read(&u,  2);
                    gpu->Read(&vv, 2);
                    v.uv1.x = (float)u  / 32768.0f;
                    v.uv1.y = (float)vv / 32768.0f;
                }
                break;

            // UV2 and UV3: skip for now (not needed for rendering; avoiding
            // MeshPart bloat until a use case requires them).
            case Semantic::UV2:
            case Semantic::UV3:
                break;

            // â”€â”€ Bone indices â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            // C# ref (GoWRknk.cs:707-805): the BoneIdx element's compCount
            // selects the encoding mode (`num46` in C#):
            //   compCount=1, fmt=Uint8 / Uint16: 4 Ã— raw indices (4-influence)
            //   compCount=2, fmt=Uint16:         7 Ã— u16 + 1 u16 pad (7-influence)
            //   compCount=3, fmt=R10G10B10:      4 Ã— u32 holding 10 Ã— 11-bit
            //                                    bone indices (10-influence)
            // We only store the first 4 in v.boneIndices regardless.
            // The index width comes from the format byte, not from compCount.
            // The game's vertex shader switches on the format code to decide
            // how to fetch an attribute, and compCount is simply how many
            // values follow. Dispatching on compCount reads a 4-byte tuple of
            // uint8 indices as an 11-bit packed block and yields nonsense
            // (bone 901 out of a 318-bone skeleton).
            //
            // Indices are GLOBAL skeleton indices, not palette-local ones:
            // measured on r_athena00, a part's distinct vertex indices are
            // exactly the set of values in its MG palette.
            case Semantic::BoneIdx: {
                const uint32_t elemBytes = ElementSize(c.format, 1);
                const int      n         = (c.compCount < 4) ? c.compCount : 4;
                uint32_t idx[4] = { 0, 0, 0, 0 };

                if (elemBytes == 1) {
                    for (int k = 0; k < n; ++k) {
                        uint8_t b = 0; gpu->Read(&b, 1); idx[k] = b;
                    }
                } else if (elemBytes == 2) {
                    for (int k = 0; k < n; ++k) {
                        uint16_t b = 0; gpu->Read(&b, 2); idx[k] = b;
                    }
                } else {
                    // Packed encoding: 11-bit indices across uint32 words.
                    uint32_t u0 = 0, u1 = 0;
                    gpu->Read(&u0, 4);
                    gpu->Read(&u1, 4);
                    idx[0] = u0 >> 21;
                    idx[1] = (u0 >> 10) & 0x7FF;
                    idx[2] = (u1 >> 31) | ((u0 & 0x3FF) << 1);
                    idx[3] = (u1 >> 20) & 0x7FF;
                }
                v.boneIndices = glm::uvec4(idx[0], idx[1], idx[2], idx[3]);
                break;
            }

            case Semantic::BoneWgt:
                if (c.format == AttrFormat::R10G10B10) {
                    // compCount tells us how many packed uint32s to read
                    // compCount=1 â†’ 1 uint32 â†’ 3 packed weights (4th implicit)
                    // compCount=2 â†’ 2 uint32s â†’ 6 weights
                    // compCount=3 â†’ 3 uint32s â†’ 9 weights
                    const float kScale = 1.0f / 1023.0f;
                    float w[9] = {};
                    int wIdx = 0;
                    for (int pack = 0; pack < c.compCount; ++pack) {
                        uint32_t u32;
                        gpu->Read(&u32, 4);
                        w[wIdx++] = (float)((u32      ) & 0x3FF) * kScale;
                        w[wIdx++] = (float)((u32 >> 10) & 0x3FF) * kScale;
                        w[wIdx++] = (float)((u32 >> 20) & 0x3FF) * kScale;
                    }
                    // For 4-influence (compCount=1): 4th = 1 - (w0+w1+w2), clamped
                    if (c.compCount == 1) {
                        float sum = w[0] + w[1] + w[2];
                        w[3] = (sum < 1.0f) ? (1.0f - sum) : 0.0f;
                    }
                    v.boneWeights = glm::vec4(w[0], w[1], w[2], w[3]);
                } else if (c.format == AttrFormat::Uint8) {
                    // 4 Ã— uint8/255
                    uint8_t b0, b1, b2, b3;
                    gpu->Read(&b0, 1);
                    gpu->Read(&b1, 1);
                    gpu->Read(&b2, 1);
                    gpu->Read(&b3, 1);
                    v.boneWeights = glm::vec4(
                        b0 / 255.0f, b1 / 255.0f,
                        b2 / 255.0f, b3 / 255.0f);
                }
                break;

            // â”€â”€ Tangent (not needed for geometry display) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            case Semantic::Tangent:
                break;

            default:
                break;
            }
        }  // vi
    }  // component loop

    return true;
}

// â”€â”€ ReadIndices â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool GOWRMeshParser::ReadIndices(std::shared_ptr<Vfs::IFile>& gpu,
                                  const SubmeshHeader& hdr,
                                  Parsers::MeshPart& part)
{
    const uint32_t idxCount = hdr.indCount;
    part.indices.resize(idxCount);

    gpu->Seek(hdr.gpuIndexOffset, SEEK_SET);

    if (hdr.indicesStride == 2) {
        for (uint32_t i = 0; i < idxCount; ++i) {
            uint16_t idx;
            gpu->Read(&idx, 2);
            part.indices[i] = idx;
        }
    } else if (hdr.indicesStride == 4) {
        for (uint32_t i = 0; i < idxCount; ++i) {
            uint32_t idx;
            gpu->Read(&idx, 4);
            part.indices[i] = idx;
        }
    } else {
        LOG_ERR("[GOWRMeshParser] Unknown index stride: %u", hdr.indicesStride);
        return false;
    }

    return true;
}

// â”€â”€ Parse (full geometry pass) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool GOWRMeshParser::Parse(std::shared_ptr<Vfs::IFile> meshFile,
                            std::shared_ptr<Vfs::IFile> gpuFile,
                            Parsers::MeshData& outData,
                            std::vector<uint32_t>* outMaterialOfPart)
{
    if (!meshFile || !meshFile->IsValid()) return false;
    if (!gpuFile  || !gpuFile->IsValid())  return false;

    const std::vector<uint32_t> smOffsets = ReadSubmeshTable(meshFile);
    const uint32_t submeshCount = static_cast<uint32_t>(smOffsets.size());
    LOG_INFO("[GOWRMeshParser] submeshCount=%u", submeshCount);
    if (submeshCount == 0) return true;

    uint32_t totalVerts = 0, totalFaces = 0;
    int      skipped = 0;

    for (uint32_t smIdx = 0; smIdx < submeshCount; ++smIdx) {
        const uint32_t base = smOffsets[smIdx];

        SubmeshHeader hdr;
        if (!ReadSubmeshHeader(meshFile, base, hdr)) { ++skipped; continue; }

        // Skip degenerate submeshes
        if (hdr.vertCount == 0 || hdr.indCount == 0 || hdr.componentCount == 0) {
            ++skipped;
            continue;
        }

        // Parse() reads geometry directly from gpuFile. That only works for
        // submeshes with internal LOD (hash == 0). External-LOD submeshes need
        // ParseWithLodPack; here we skip them rather than read garbage.
        if (hdr.meshHash != 0) {
            ++skipped;
            continue;
        }

        std::vector<ComponentDesc> comps;
        ReadComponents(meshFile, hdr, comps);

        std::vector<uint32_t> bufOffsets;
        ReadBufferOffsets(meshFile, hdr, bufOffsets);

        // Validate buffer indices
        bool valid = true;
        for (const auto& c : comps) {
            if (c.bufferIdx >= bufOffsets.size()) {
                LOG_WARN("[GOWRMeshParser] SM#%u: comp semantic=%u bufIdx=%u >= bufCount=%u â€” skip",
                         smIdx, (uint8_t)c.semantic, c.bufferIdx, hdr.bufferCount);
                valid = false;
                break;
            }
        }
        if (!valid) { ++skipped; continue; }

        // Skip if no UV channel (can't texture without UVs)
        bool hasUV = false;
        for (const auto& c : comps)
            if (c.semantic == Semantic::UV0) { hasUV = true; break; }
        if (!hasUV) {
            LOG_DEBUG("[GOWRMeshParser] SM#%u dropped: no UV0 channel", smIdx);
            ++skipped; continue;
        }

        // Vertex-layout diagnostics. The multi-buffer path ignores each
        // component's byteOffset, so this dump is what tells us whether two
        // components share a buffer (which would make that assumption wrong).
        LOG_DEBUG("[GOWRMeshParser] SM#%u @0x%X: %u v, %u f, bufCount=%u idxStride=%u bytesPerVert=%u comps=%u",
                  smIdx, base, hdr.vertCount, hdr.faceCount, hdr.bufferCount,
                  hdr.indicesStride, hdr.bytesPerVertex, hdr.componentCount);
        for (size_t ci = 0; ci < comps.size(); ++ci) {
            const auto& c = comps[ci];
            LOG_DEBUG("[GOWRMeshParser]   comp[%zu] sem=%u fmt=%u cnt=%u byteOff=%u buf=%u",
                      ci, (unsigned)c.semantic, (unsigned)c.format,
                      (unsigned)c.compCount, (unsigned)c.byteOffset, c.bufferIdx);
        }

        Parsers::MeshPart part;
        part.materialId = smIdx;
        part.meshHash   = hdr.meshHash;

        if (!ReadVertices(gpuFile, hdr, comps, bufOffsets, part)) { ++skipped; continue; }
        if (!ReadIndices (gpuFile, hdr, part))                    { ++skipped; continue; }

        totalVerts += hdr.vertCount;
        totalFaces += hdr.faceCount;
        if (outMaterialOfPart) outMaterialOfPart->push_back(hdr.materialIndex);
        outData.parts.push_back(std::move(part));
    }

    LOG_INFO("[GOWRMeshParser] Done: %zu submeshes exported (%d skipped), %u verts, %u faces",
             outData.parts.size(), skipped, totalVerts, totalFaces);

    // Bounding box from actual vertex data
    glm::vec3 bmin( 1e9f), bmax(-1e9f);
    for (const auto& p : outData.parts)
        for (const auto& v : p.vertices) {
            bmin = glm::min(bmin, v.position);
            bmax = glm::max(bmax, v.position);
        }
    outData.bounds.min = bmin;
    outData.bounds.max = bmax;

    return !outData.parts.empty();
}

// â”€â”€ ParseMeshDefn (header-only, for tree inspector + lodpack lookup) â”€â”€â”€â”€â”€â”€â”€
bool GOWRMeshParser::ParseMeshDefn(std::shared_ptr<Vfs::IFile> defFile,
                                    std::shared_ptr<Vfs::IFile> /*lodpackFile*/,
                                    std::vector<std::shared_ptr<GpuMesh>>& /*outMeshes*/)
{
    if (!defFile || !defFile->IsValid()) return false;

    const std::vector<uint32_t> offsets = ReadSubmeshTable(defFile);
    const uint32_t submeshCount = static_cast<uint32_t>(offsets.size());

    for (uint32_t i = 0; i < submeshCount; ++i) {
        SubmeshHeader hdr;
        ReadSubmeshHeader(defFile, offsets[i], hdr);
        LOG_INFO("[GOWRMeshParser] SM#%u @0x%X: %u v, %u f, %u comp, %u bufs, "
                 "idxStride=%u, hash=0x%016llX",
                 i, offsets[i],
                 hdr.vertCount, hdr.faceCount,
                 hdr.componentCount, hdr.bufferCount,
                 hdr.indicesStride,
                 (unsigned long long)hdr.meshHash);
    }

    return true;
}

// â”€â”€ ParseWithLodPack (full parse using a LodPackIndex) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// This is the main entry point for production use.
// It mirrors the C# Main() loop: for each submesh, looks up the LOD blob in
// the lodpack, wraps it in a MemoryFile, and calls the standard ReadVertices/
// ReadIndices pipeline.
bool GOWRMeshParser::ParseWithLodPack(std::shared_ptr<Vfs::IFile>    meshFile,
                                       std::shared_ptr<Vfs::IFile>    gpuFile,
                                       const LodPackIndex&       lodIdx,
                                       Parsers::MeshData&                 outData,
                                       std::vector<uint32_t>* outMaterialOfPart)
{
    if (!meshFile || !meshFile->IsValid()) return false;

    const std::vector<uint32_t> smOffsets = ReadSubmeshTable(meshFile);
    const uint32_t submeshCount = static_cast<uint32_t>(smOffsets.size());
    if (submeshCount == 0) return true;

    uint32_t totalVerts = 0, totalFaces = 0;
    int      skipped = 0;

    for (uint32_t smIdx = 0; smIdx < submeshCount; ++smIdx) {
        const uint32_t base = smOffsets[smIdx];

        SubmeshHeader hdr;
        if (!ReadSubmeshHeader(meshFile, base, hdr)) { ++skipped; continue; }

        if (hdr.vertCount == 0 || hdr.indCount == 0 || hdr.componentCount == 0) {
            ++skipped;
            continue;
        }

        // â”€â”€ LOD source selection â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        // meshHash == 0 â†’ data is embedded in the GPU file (internal LOD)
        // meshHash != 0 â†’ look up in lodpack
        std::shared_ptr<Vfs::IFile> lodFile;

        if (hdr.meshHash == 0) {
            // Internal: use the gpuFile as-is (it IS the LOD blob)
            lodFile = gpuFile;
            LOG_INFO("[GOWRMeshParser] SM#%u: internal LOD (hash=0, using gpuFile)", smIdx);
        } else {
            const LodEntry* entry = lodIdx.Find(hdr.meshHash);
            if (!entry) {
                LOG_WARN("[GOWRMeshParser] SM#%u: LOD not found for hash=0x%016llX â€” skipping",
                         smIdx, (unsigned long long)hdr.meshHash);
                ++skipped;
                continue;
            }

            std::vector<uint8_t> blob;
            if (!lodIdx.ReadBlob(*entry, blob)) {
                LOG_WARN("[GOWRMeshParser] SM#%u: failed to read LOD blob â€” skipping", smIdx);
                ++skipped;
                continue;
            }

            LOG_INFO("[GOWRMeshParser] SM#%u: LOD pack[%d] off=0x%llX size=%d",
                     smIdx, entry->packIdx,
                     (unsigned long long)entry->offset, entry->size);

            lodFile = std::make_shared<Vfs::MemoryFile>(std::move(blob));
        }

        std::vector<ComponentDesc> comps;
        ReadComponents(meshFile, hdr, comps);

        std::vector<uint32_t> bufOffsets;
        ReadBufferOffsets(meshFile, hdr, bufOffsets);

        bool valid = true;
        for (const auto& c : comps) {
            if (c.bufferIdx >= bufOffsets.size()) { valid = false; break; }
        }
        if (!valid) { ++skipped; continue; }

        bool hasUV = false;
        for (const auto& c : comps)
            if (c.semantic == Semantic::UV0) { hasUV = true; break; }
        if (!hasUV) { ++skipped; continue; }

        Parsers::MeshPart part;
        part.materialId = smIdx;
        part.meshHash   = hdr.meshHash;

        if (!ReadVertices(lodFile, hdr, comps, bufOffsets, part)) { ++skipped; continue; }
        if (!ReadIndices (lodFile, hdr, part))                    { ++skipped; continue; }

        totalVerts += hdr.vertCount;
        totalFaces += hdr.faceCount;
        if (outMaterialOfPart) outMaterialOfPart->push_back(hdr.materialIndex);
        outData.parts.push_back(std::move(part));
    }

    LOG_INFO("[GOWRMeshParser] ParseWithLodPack: %zu parts (%d skipped), %u verts, %u faces",
             outData.parts.size(), skipped, totalVerts, totalFaces);

    glm::vec3 bmin( 1e9f), bmax(-1e9f);
    for (const auto& p : outData.parts)
        for (const auto& v : p.vertices) {
            bmin = glm::min(bmin, v.position);
            bmax = glm::max(bmax, v.position);
        }
    outData.bounds.min = bmin;
    outData.bounds.max = bmax;

    return !outData.parts.empty();
}

} // namespace Onyx
