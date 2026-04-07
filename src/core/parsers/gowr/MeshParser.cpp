#include "MeshParser.h"
#include "LodPackIndex.h"
#include "core/Logger.h"
#include "core/vfs/MemoryFile.h"
#include <cstring>
#include <algorithm>

// ── MeshParser.cpp ─────────────────────────────────────────────────────────
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

namespace GOW {

// ── ElementSize ────────────────────────────────────────────────────────────
static uint32_t ElementSize(GOWRMeshParser::AttrFormat fmt, uint8_t compCount) {
    switch (fmt) {
        case GOWRMeshParser::AttrFormat::Float32:   return compCount * 4;
        case GOWRMeshParser::AttrFormat::R10G10B10: return 4;
        case GOWRMeshParser::AttrFormat::Uint16:    return compCount * 2;
        case GOWRMeshParser::AttrFormat::Int16:     return compCount * 2;
        case GOWRMeshParser::AttrFormat::Uint8:     return compCount * 1;
        default:                                    return compCount * 4;
    }
}

// ── ReadSubmeshHeader ──────────────────────────────────────────────────────
bool GOWRMeshParser::ReadSubmeshHeader(std::shared_ptr<IFile>& f,
                                       uint32_t base,
                                       SubmeshHeader& h)
{
    f->Seek(base + 0x10, SEEK_SET);
    f->Read(&h.extent, 12);

    f->Seek(base + 0x1C, SEEK_SET);
    f->Read(&h.origin, 12);

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

    f->Seek(base + 0x80, SEEK_SET);
    f->Read(&h.bufferCount,    1);
    f->Read(&h.indicesStride,  1);
    f->Read(&h.bytesPerVertex, 1);
    f->Read(&h.componentCount, 1);

    h.componentOffsetAbs = base + compOffRel;
    h.bufOffsetsAbs      = base + bufOffRel;

    return true;
}

// ── ReadComponents ─────────────────────────────────────────────────────────
bool GOWRMeshParser::ReadComponents(std::shared_ptr<IFile>& f,
                                     const SubmeshHeader& hdr,
                                     std::vector<ComponentDesc>& out)
{
    out.resize(hdr.componentCount);
    f->Seek(hdr.componentOffsetAbs, SEEK_SET);
    for (auto& c : out) {
        uint8_t s, fmt, cnt, off;
        uint32_t bufIdx;
        f->Read(&s,      1);
        f->Read(&fmt,    1);
        f->Read(&cnt,    1);
        f->Read(&off,    1);
        f->Read(&bufIdx, 4);
        c.semantic   = static_cast<Semantic>(s);
        c.format     = static_cast<AttrFormat>(fmt);
        c.compCount  = cnt;
        c.byteOffset = off;
        c.bufferIdx  = bufIdx;
    }
    return true;
}

// ── ReadBufferOffsets ──────────────────────────────────────────────────────
bool GOWRMeshParser::ReadBufferOffsets(std::shared_ptr<IFile>& f,
                                        const SubmeshHeader& hdr,
                                        std::vector<uint32_t>& out)
{
    out.resize(hdr.bufferCount);
    f->Seek(hdr.bufOffsetsAbs, SEEK_SET);
    for (auto& o : out)
        f->Read(&o, 4);
    return true;
}

// ── ReadVertices ───────────────────────────────────────────────────────────
bool GOWRMeshParser::ReadVertices(std::shared_ptr<IFile>& gpu,
                                   const SubmeshHeader& hdr,
                                   const std::vector<ComponentDesc>& comps,
                                   const std::vector<uint32_t>& bufOffsets,
                                   MeshPart& part)
{
    const uint32_t N = hdr.vertCount;
    part.vertices.resize(N);

    // Initialise all skin data to zero (neutral: weight=0, index=0)
    for (auto& v : part.vertices) {
        v.boneWeights = glm::vec4(0.0f);
        v.boneIndices = glm::uvec4(0u);
    }

    for (const auto& c : comps) {
        if (c.bufferIdx >= bufOffsets.size()) continue;

        const uint32_t streamBase = bufOffsets[c.bufferIdx];
        const uint32_t elemBytes  = ElementSize(c.format, c.compCount);
        const uint32_t stride     = (hdr.bufferCount == 1)
                                      ? hdr.bytesPerVertex
                                      : elemBytes;

        for (uint32_t vi = 0; vi < N; ++vi) {
            const uint32_t vertOff = streamBase
                                   + vi * stride
                                   + (hdr.bufferCount == 1 ? c.byteOffset : 0u);
            gpu->Seek(vertOff, SEEK_SET);

            GpuVertex& v = part.vertices[vi];

            switch (c.semantic) {

            // ── Position ────────────────────────────────────────────────
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

            // ── Normal ──────────────────────────────────────────────────
            case Semantic::Normal:
                if (c.format == AttrFormat::R10G10B10) {
                    // 3 × 10-bit signed packed in uint32.
                    // Confirmed bit layout from C# log:
                    //   norm[0] = (0.5449, 0.7207, -0.4238)
                    // Encoding: X=[9:0], Y=[19:10], Z=[29:20], W=[31:30] ignored
                    // Scale: /511.0f  (same as C#: /511.9f approximated)
                    uint32_t packed;
                    gpu->Read(&packed, 4);
                    auto unpack10 = [](uint32_t p, int shift) -> float {
                        int32_t v = (p >> shift) & 0x3FF;
                        if (v & 0x200) v |= ~0x3FF;  // sign-extend
                        return (float)v / 511.0f;
                    };
                    v.normal.x = unpack10(packed,  0);
                    v.normal.y = unpack10(packed, 10);
                    v.normal.z = unpack10(packed, 20);
                    v.normal   = glm::normalize(v.normal);
                }
                break;

            // ── UV0 (primary UV, stream UV1 in C# naming) ───────────────
            case Semantic::UV0:
                if (c.format == AttrFormat::Float32 && c.compCount >= 2) {
                    gpu->Read(&v.uv.x, 4);
                    gpu->Read(&v.uv.y, 4);
                } else if (c.format == AttrFormat::Uint16 && c.compCount >= 2) {
                    // unorm16: raw/65535
                    uint16_t u, vv;
                    gpu->Read(&u,  2);
                    gpu->Read(&vv, 2);
                    v.uv.x = (float)u  / 65535.0f;
                    v.uv.y = (float)vv / 65535.0f;
                } else if (c.format == AttrFormat::Int16 && c.compCount >= 2) {
                    // snorm16: raw/32767
                    int16_t u, vv;
                    gpu->Read(&u,  2);
                    gpu->Read(&vv, 2);
                    v.uv.x = (float)u  / 32767.0f;
                    v.uv.y = (float)vv / 32767.0f;
                }
                break;

            // ── UV1 / UV2 / UV3 (lightmap, detail, etc.) ────────────────
            // Stored in extra UV channels. We carry UV1 into uv1; UV2/UV3
            // are stored in extra channels if MeshPart supports them.
            // For geometry display only UV0 is strictly needed, but we
            // read and store UV1 since some materials require it (lightmap).
            case Semantic::UV1:
                if (c.format == AttrFormat::Float32 && c.compCount >= 2) {
                    gpu->Read(&v.uv1.x, 4);
                    gpu->Read(&v.uv1.y, 4);
                } else if (c.format == AttrFormat::Uint16 && c.compCount >= 2) {
                    uint16_t u, vv;
                    gpu->Read(&u,  2);
                    gpu->Read(&vv, 2);
                    v.uv1.x = (float)u  / 65535.0f;
                    v.uv1.y = (float)vv / 65535.0f;
                } else if (c.format == AttrFormat::Int16 && c.compCount >= 2) {
                    int16_t u, vv;
                    gpu->Read(&u,  2);
                    gpu->Read(&vv, 2);
                    v.uv1.x = (float)u  / 32767.0f;
                    v.uv1.y = (float)vv / 32767.0f;
                }
                break;

            // UV2 and UV3: skip for now (not needed for rendering; avoiding
            // MeshPart bloat until a use case requires them).
            case Semantic::UV2:
            case Semantic::UV3:
                break;

            // ── Bone indices ─────────────────────────────────────────────
            // Two sub-formats observed in the Ragnarök log:
            //   fmt=4 (Uint8):  4 × uint8  bone indices  (GoW 2018 style)
            //   fmt=8 (Uint16): 4 × uint16 bone indices  (Ragnarök, wider skeleton)
            // C# code: num49 == 4 → 4B uint8 / num49 == 8 → 4B uint16
            case Semantic::BoneIdx:
                if (c.format == AttrFormat::Uint8) {
                    uint8_t b0, b1, b2, b3;
                    gpu->Read(&b0, 1);
                    gpu->Read(&b1, 1);
                    gpu->Read(&b2, 1);
                    gpu->Read(&b3, 1);
                    v.boneIndices = glm::uvec4(b0, b1, b2, b3);
                } else if (c.format == AttrFormat::Uint16) {
                    uint16_t b0, b1, b2, b3;
                    gpu->Read(&b0, 2);
                    gpu->Read(&b1, 2);
                    gpu->Read(&b2, 2);
                    gpu->Read(&b3, 2);
                    v.boneIndices = glm::uvec4(b0, b1, b2, b3);
                }
                break;

            // ── Bone weights ─────────────────────────────────────────────
            // Three sub-formats (directly mirroring C# num46 cases):
            //
            //   compCount=1, fmt=R10G10B10 (num46==3, 10 influences):
            //     Three consecutive uint32s, each holding 3 × 10-bit weights.
            //     Total 9 weights packed; 10th is implied (1 - sum).
            //     We store only the first 4 in boneWeights.
            //
            //   compCount=1, fmt=R10G10B10 (num46==2, 6 influences):
            //     Two uint32s × 3 weights each. We store first 4.
            //
            //   compCount=1, fmt=R10G10B10 (num46==1, 4 influences):
            //     Standard: single uint32 with 3 packed 10-bit values.
            //     4th weight = 1 - (w0+w1+w2).
            //
            //   compCount=1, fmt=Uint8 (byte weights, GoW 2018):
            //     4 bytes, each /255.
            case Semantic::BoneWgt:
                if (c.format == AttrFormat::R10G10B10) {
                    // compCount tells us how many packed uint32s to read
                    // compCount=1 → 1 uint32 → 3 packed weights (4th implicit)
                    // compCount=2 → 2 uint32s → 6 weights
                    // compCount=3 → 3 uint32s → 9 weights
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
                    // 4 × uint8/255
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

            // ── Tangent (not needed for geometry display) ────────────────
            case Semantic::Tangent:
                break;

            default:
                break;
            }
        }  // vi
    }  // component loop

    return true;
}

// ── ReadIndices ────────────────────────────────────────────────────────────
bool GOWRMeshParser::ReadIndices(std::shared_ptr<IFile>& gpu,
                                  const SubmeshHeader& hdr,
                                  MeshPart& part)
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

// ── Parse (full geometry pass) ─────────────────────────────────────────────
bool GOWRMeshParser::Parse(std::shared_ptr<IFile> meshFile,
                            std::shared_ptr<IFile> gpuFile,
                            MeshData& outData)
{
    if (!meshFile || !meshFile->IsValid()) return false;
    if (!gpuFile  || !gpuFile->IsValid())  return false;

    // Submesh count: uint16 at +0x10
    meshFile->Seek(0x10, SEEK_SET);
    uint16_t submeshCount = 0;
    meshFile->Read(&submeshCount, 2);

    LOG_INFO("[GOWRMeshParser] submeshCount=%u", submeshCount);
    if (submeshCount == 0) return true;

    // Offset table at 0x40. Each entry:  absolute = pos + readValue
    std::vector<uint32_t> smOffsets(submeshCount);
    for (uint32_t i = 0; i < submeshCount; ++i) {
        const uint32_t pos = 0x40 + i * 4;
        uint32_t rel;
        meshFile->Seek(pos, SEEK_SET);
        meshFile->Read(&rel, 4);
        smOffsets[i] = pos + rel;
    }

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

        // If meshHash is 0 → LOD data is embedded inline in the GPU file
        // (no lodpack lookup needed). The gpuFile passed in is already correct.
        // If meshHash != 0 → caller should have already resolved the LOD blob
        // via LodPackIndex and wrapped it in a MemoryFile before calling Parse().
        // (See GOWRLoaders.cpp for how this is orchestrated.)

        std::vector<ComponentDesc> comps;
        ReadComponents(meshFile, hdr, comps);

        std::vector<uint32_t> bufOffsets;
        ReadBufferOffsets(meshFile, hdr, bufOffsets);

        // Validate buffer indices
        bool valid = true;
        for (const auto& c : comps) {
            if (c.bufferIdx >= bufOffsets.size()) {
                LOG_WARN("[GOWRMeshParser] SM#%u: comp semantic=%u bufIdx=%u >= bufCount=%u — skip",
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
        if (!hasUV) { ++skipped; continue; }

        if (smIdx < 3) {
            LOG_INFO("[GOWRMeshParser] SM#%u @0x%X: %u v, %u f, bufCount=%u stride=%u",
                     smIdx, base, hdr.vertCount, hdr.faceCount,
                     hdr.bufferCount, hdr.indicesStride);
        }

        MeshPart part;
        part.materialId = smIdx;

        if (!ReadVertices(gpuFile, hdr, comps, bufOffsets, part)) { ++skipped; continue; }
        if (!ReadIndices (gpuFile, hdr, part))                    { ++skipped; continue; }

        totalVerts += hdr.vertCount;
        totalFaces += hdr.faceCount;
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

// ── ParseMeshDefn (header-only, for tree inspector + lodpack lookup) ───────
bool GOWRMeshParser::ParseMeshDefn(std::shared_ptr<IFile> defFile,
                                    std::shared_ptr<IFile> /*lodpackFile*/,
                                    std::vector<std::shared_ptr<GpuMesh>>& /*outMeshes*/)
{
    if (!defFile || !defFile->IsValid()) return false;

    defFile->Seek(0x10, SEEK_SET);
    uint16_t submeshCount = 0;
    defFile->Read(&submeshCount, 2);

    std::vector<uint32_t> offsets(submeshCount);
    for (uint32_t i = 0; i < submeshCount; ++i) {
        const uint32_t pos = 0x40 + i * 4;
        uint32_t rel;
        defFile->Seek(pos, SEEK_SET);
        defFile->Read(&rel, 4);
        offsets[i] = pos + rel;
    }

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

// ── ParseWithLodPack (full parse using a LodPackIndex) ─────────────────────
// This is the main entry point for production use.
// It mirrors the C# Main() loop: for each submesh, looks up the LOD blob in
// the lodpack, wraps it in a MemoryFile, and calls the standard ReadVertices/
// ReadIndices pipeline.
bool GOWRMeshParser::ParseWithLodPack(std::shared_ptr<IFile>    meshFile,
                                       std::shared_ptr<IFile>    gpuFile,
                                       const LodPackIndex&       lodIdx,
                                       MeshData&                 outData)
{
    if (!meshFile || !meshFile->IsValid()) return false;

    meshFile->Seek(0x10, SEEK_SET);
    uint16_t submeshCount = 0;
    meshFile->Read(&submeshCount, 2);

    if (submeshCount == 0) return true;

    std::vector<uint32_t> smOffsets(submeshCount);
    for (uint32_t i = 0; i < submeshCount; ++i) {
        const uint32_t pos = 0x40 + i * 4;
        uint32_t rel;
        meshFile->Seek(pos, SEEK_SET);
        meshFile->Read(&rel, 4);
        smOffsets[i] = pos + rel;
    }

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

        // ── LOD source selection ─────────────────────────────────────────
        // meshHash == 0 → data is embedded in the GPU file (internal LOD)
        // meshHash != 0 → look up in lodpack
        std::shared_ptr<IFile> lodFile;

        if (hdr.meshHash == 0) {
            // Internal: use the gpuFile as-is (it IS the LOD blob)
            lodFile = gpuFile;
            LOG_INFO("[GOWRMeshParser] SM#%u: internal LOD (hash=0, using gpuFile)", smIdx);
        } else {
            const LodEntry* entry = lodIdx.Find(hdr.meshHash);
            if (!entry) {
                LOG_WARN("[GOWRMeshParser] SM#%u: LOD not found for hash=0x%016llX — skipping",
                         smIdx, (unsigned long long)hdr.meshHash);
                ++skipped;
                continue;
            }

            std::vector<uint8_t> blob;
            if (!lodIdx.ReadBlob(*entry, blob)) {
                LOG_WARN("[GOWRMeshParser] SM#%u: failed to read LOD blob — skipping", smIdx);
                ++skipped;
                continue;
            }

            LOG_INFO("[GOWRMeshParser] SM#%u: LOD pack[%d] off=0x%llX size=%d",
                     smIdx, entry->packIdx,
                     (unsigned long long)entry->offset, entry->size);

            lodFile = std::make_shared<MemoryFile>(std::move(blob));
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

        MeshPart part;
        part.materialId = smIdx;

        if (!ReadVertices(lodFile, hdr, comps, bufOffsets, part)) { ++skipped; continue; }
        if (!ReadIndices (lodFile, hdr, part))                    { ++skipped; continue; }

        totalVerts += hdr.vertCount;
        totalFaces += hdr.faceCount;
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

} // namespace GOW