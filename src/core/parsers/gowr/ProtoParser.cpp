#include "ProtoParser.h"
#include "core/Logger.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <cstdio>

// ── ProtoParser.cpp ────────────────────────────────────────────────────────
// goProto* file layout (port of GoWRknk.cs:210-285):
//
//   +0x00   16 bytes header (unused fields)
//   +0x10   int32 boneCount
//   +0x14   int32 (unused)
//
//   +0x18   bone entry table (boneCount × 8 bytes):
//             int16 (skip)
//             int16 (skip)
//             int16 (skip)
//             int16 parentIdx  ← used
//
//   +0x18 + 8*N     padding (8 * N bytes)
//   +0x18 + 16*N    padding (16 * N bytes)
//
//   then:
//             int64 (skip)
//             int32 × 4 (skip)
//             64 bytes (skip)
//
//   local transform table (boneCount × 64 bytes):
//             float[3][4]  rotation 3x3 (last column ignored)  = 48 bytes
//             float[4]     position (last component ignored)   = 16 bytes

namespace GOW {

std::shared_ptr<ObjectData> GOWRProtoParser::Parse(std::shared_ptr<IFile> file) {
    if (!file || !file->IsValid()) return nullptr;

    auto obj = std::make_shared<ObjectData>();

    file->Seek(0x10, SEEK_SET);
    int32_t boneCount = 0;
    file->Read(&boneCount, 4);
    int32_t unused = 0;
    file->Read(&unused, 4);

    if (boneCount <= 0 || boneCount > 4096) {
        LOG_WARN("[GOWRProtoParser] Implausible bone count: %d", boneCount);
        return nullptr;
    }

    LOG_INFO("[GOWRProtoParser] Bone count: %d", boneCount);

    obj->joints.resize(boneCount);

    // ── Parent table @ +0x18 ────────────────────────────────────────────────
    for (int j = 0; j < boneCount; ++j) {
        int16_t a, b, c, parent;
        file->Read(&a, 2);
        file->Read(&b, 2);
        file->Read(&c, 2);
        file->Read(&parent, 2);
        obj->joints[j].id     = (int16_t)j;
        obj->joints[j].parent = parent;
        obj->joints[j].invId  = (int16_t)j;
        obj->joints[j].isSkinned = true;
        char nameBuf[24];
        std::snprintf(nameBuf, sizeof(nameBuf), "bone_%03d", j);
        obj->joints[j].name = nameBuf;
    }

    // Skip 8*N then 16*N padding tables
    file->Seek(8 * boneCount, SEEK_CUR);
    file->Seek(16 * boneCount, SEEK_CUR);

    // Skip int64 + 4 int32 + 64 bytes header block
    int64_t skip64; int32_t skip32;
    file->Read(&skip64, 8);
    file->Read(&skip32, 4); file->Read(&skip32, 4);
    file->Read(&skip32, 4); file->Read(&skip32, 4);
    file->Seek(64, SEEK_CUR);

    // ── Table A: local parent→joint matrices (mat4, COLUMN-major, 64B/bone) ──
    // Confirmed via Ghidra FUN_140699110 matmul ordering: proto stores each mat
    // as 4 columns of 4 floats each. Runtime memcpies Table A directly into
    // skel[+0x90] and consumes column-major in the compose pass.
    //
    // Old code transposed (reading as row-major). That corrupted rotations into
    // their transpose (= inverse for orthogonal R) and put translation in the
    // wrong slots, producing the "spider" bone debug pattern.
    obj->matrixes1.resize(boneCount);
    obj->matrixes3.resize(boneCount);
    obj->vectors4.resize(boneCount, glm::vec4(0));
    obj->vectors5.resize(boneCount, glm::ivec4(0));
    obj->vectors6.resize(boneCount, glm::vec4(1.0f));

    std::vector<glm::mat4> local(boneCount, glm::mat4(1.0f));

    for (int j = 0; j < boneCount; ++j) {
        // 4 columns × 4 floats each, sequential. Last float of each column is
        // padding/homogeneous (0 for basis vectors, 1 for translation column).
        glm::mat4 M(1.0f);
        file->Read(&M[0].x, 4); file->Read(&M[0].y, 4); file->Read(&M[0].z, 4); file->Read(&M[0].w, 4);
        file->Read(&M[1].x, 4); file->Read(&M[1].y, 4); file->Read(&M[1].z, 4); file->Read(&M[1].w, 4);
        file->Read(&M[2].x, 4); file->Read(&M[2].y, 4); file->Read(&M[2].z, 4); file->Read(&M[2].w, 4);
        file->Read(&M[3].x, 4); file->Read(&M[3].y, 4); file->Read(&M[3].z, 4); file->Read(&M[3].w, 4);

        local[j]                     = M;
        obj->matrixes1[j]            = M;
        obj->vectors4[j]             = glm::vec4(M[3].x, M[3].y, M[3].z, 0);
        obj->joints[j].parentToJoint = M;
    }

    // ── Hierarchical world rest pose (column-major matmul) ──────────────────
    // Mirror of FUN_140699110: world[i] = world[parent[i]] * local[i].
    // Bones are stored in topological order (parent index < child index in
    // every example we have), so a single forward pass suffices.
    std::vector<glm::mat4> composedWorld(boneCount, glm::mat4(1.0f));
    for (int j = 0; j < boneCount; ++j) {
        int p = obj->joints[j].parent;
        composedWorld[j] = (p < 0 || p >= boneCount)
            ? local[j]
            : (composedWorld[p] * local[j]);
    }

    // ── Table B: read raw (column-major), purpose unknown at runtime ────────
    // FUN_1406ed6b0 copies only Table A into the runtime buffer; Table B is
    // never consumed by the skinning pipeline we traced. We read it to keep
    // file-position correct for any downstream consumer, but do not use it
    // for renderMat / palette.
    for (int j = 0; j < boneCount; ++j) {
        glm::mat4 unused(1.0f);
        file->Read(&unused[0].x, 4); file->Read(&unused[0].y, 4); file->Read(&unused[0].z, 4); file->Read(&unused[0].w, 4);
        file->Read(&unused[1].x, 4); file->Read(&unused[1].y, 4); file->Read(&unused[1].z, 4); file->Read(&unused[1].w, 4);
        file->Read(&unused[2].x, 4); file->Read(&unused[2].y, 4); file->Read(&unused[2].z, 4); file->Read(&unused[2].w, 4);
        file->Read(&unused[3].x, 4); file->Read(&unused[3].y, 4); file->Read(&unused[3].z, 4); file->Read(&unused[3].w, 4);

        obj->joints[j].renderMat      = composedWorld[j];
        obj->joints[j].bindToJointMat = glm::mat4(1.0f);
        obj->matrixes3[j]             = glm::mat4(1.0f);
    }

    return obj;
}

} // namespace GOW
