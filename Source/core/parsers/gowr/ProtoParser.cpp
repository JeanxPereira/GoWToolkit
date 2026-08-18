#include "ProtoParser.h"
#include <Onyx/Services/Logger.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <cstdio>

// â”€â”€ ProtoParser.cpp â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// goProto* file layout (port of GoWRknk.cs:210-285):
//
//   +0x00   16 bytes header (unused fields)
//   +0x10   int32 boneCount
//   +0x14   int32 (unused)
//
//   +0x18   bone entry table (boneCount Ã— 8 bytes):
//             int16 (skip)
//             int16 (skip)
//             int16 (skip)
//             int16 parentIdx  â† used
//
//   +0x18 + 8*N     padding (8 * N bytes)
//   +0x18 + 16*N    padding (16 * N bytes)
//
//   then:
//             int64 (skip)
//             int32 Ã— 4 (skip)
//             64 bytes (skip)
//
//   local transform table (boneCount Ã— 64 bytes):
//             float[3][4]  rotation 3x3 (last column ignored)  = 48 bytes
//             float[4]     position (last component ignored)   = 16 bytes

namespace Onyx {

std::shared_ptr<Parsers::ObjectData> GOWRProtoParser::Parse(std::shared_ptr<Vfs::IFile> file) {
    if (!file || !file->IsValid()) return nullptr;

    auto obj = std::make_shared<Parsers::ObjectData>();

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

    // â”€â”€ Parent table @ +0x18 â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

    // â”€â”€ Table A: local parentâ†’joint matrices (mat4, COLUMN-major, 64B/bone) â”€â”€
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
        // 4 columns Ã— 4 floats each, sequential. Last float of each column is
        // padding/homogeneous (0 for basis vectors, 1 for translation column).
        glm::mat4 M(1.0f);
        file->Read(&M[0].x, 4); file->Read(&M[0].y, 4); file->Read(&M[0].z, 4); file->Read(&M[0].w, 4);
        file->Read(&M[1].x, 4); file->Read(&M[1].y, 4); file->Read(&M[1].z, 4); file->Read(&M[1].w, 4);
        file->Read(&M[2].x, 4); file->Read(&M[2].y, 4); file->Read(&M[2].z, 4); file->Read(&M[2].w, 4);
        file->Read(&M[3].x, 4); file->Read(&M[3].y, 4); file->Read(&M[3].z, 4); file->Read(&M[3].w, 4);

        local[j]                     = M;
        obj->matrixes1[j]            = M;
        obj->joints[j].parentToJoint = M;

        // The renderer rebuilds each local transform from the TRS vectors
        // (T * R * S) rather than from parentToJoint, so the matrix has to
        // be decomposed into them. Leaving the rotation at zero yields a
        // translation-only skeleton, which is what stretched the model
        // along its bone chain.
        glm::vec3 axisX(M[0]), axisY(M[1]), axisZ(M[2]);
        const float sx = glm::length(axisX);
        const float sy = glm::length(axisY);
        const float sz = glm::length(axisZ);

        glm::mat3 R(1.0f);
        if (sx > 1e-6f && sy > 1e-6f && sz > 1e-6f) {
            R = glm::mat3(axisX / sx, axisY / sy, axisZ / sz);
        }

        // Q.14 quaternion is what the quaternion path of the renderer's
        // TRS builder expects; it renormalises, so rounding is harmless.
        const glm::quat q = glm::normalize(glm::quat_cast(R));
        auto q14 = [](float v) {
            return static_cast<int>(std::lround(v * 16384.0f));
        };

        obj->vectors4[j] = glm::vec4(M[3].x, M[3].y, M[3].z, 0.0f);
        obj->vectors5[j] = glm::ivec4(q14(q.x), q14(q.y), q14(q.z), q14(q.w));
        obj->vectors6[j] = glm::vec4(sx > 1e-6f ? sx : 1.0f,
                                     sy > 1e-6f ? sy : 1.0f,
                                     sz > 1e-6f ? sz : 1.0f, 0.0f);
        obj->joints[j].isQuaternion = true;
    }

    // â”€â”€ Hierarchical world rest pose (column-major matmul) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // Mirror of FUN_140699110: world[i] = world[parent[i]] * local[i].
    //
    // Bones are NOT in topological order: goProtoathena10 opens with
    // parent(0) = 1 and parent(1) = 3, so a single forward pass would
    // compose a child against an unresolved parent. Resolve each chain on
    // demand instead, memoising as we go.
    std::vector<glm::mat4> composedWorld(boneCount, glm::mat4(1.0f));
    std::vector<uint8_t>   resolved(boneCount, 0);

    for (int start = 0; start < boneCount; ++start) {
        if (resolved[start]) continue;

        // Walk up to the first resolved ancestor (or a root), guarding
        // against a malformed file that loops a bone back onto itself.
        std::vector<int> chain;
        std::vector<uint8_t> onChain(boneCount, 0);
        int j = start;
        while (j >= 0 && j < boneCount && !resolved[j] && !onChain[j]) {
            onChain[j] = 1;
            chain.push_back(j);
            j = obj->joints[j].parent;
        }
        if (j >= 0 && j < boneCount && onChain[j]) {
            LOG_WARN("[GOWRProtoParser] bone %d sits on a parent cycle - "
                     "treating it as a root", j);
        }

        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            const int b = *it;
            const int p = obj->joints[b].parent;
            composedWorld[b] = (p < 0 || p >= boneCount || !resolved[p])
                ? local[b]
                : (composedWorld[p] * local[b]);
            resolved[b] = 1;
        }
    }

    // â”€â”€ Table B: read raw (column-major), purpose unknown at runtime â”€â”€â”€â”€â”€â”€â”€â”€
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

        // The skinning palette is worldRestPose * bindToJoint, so bindToJoint
        // has to be the inverse of the rest pose: only then does the palette
        // collapse to identity at rest and leave the mesh as authored.
        // Leaving it as identity transforms every vertex by its bone's world
        // matrix a second time, which stretches the model along the skeleton.
        //
        // Table B above is not that inverse - checked on goProtoathena10,
        // where world[j] * B[j] is identity for only the 5 root bones - so
        // the inverse is computed rather than read.
        obj->joints[j].renderMat      = composedWorld[j];
        obj->joints[j].bindToJointMat = glm::inverse(composedWorld[j]);
        obj->matrixes3[j]             = obj->joints[j].bindToJointMat;
    }

    return obj;
}

} // namespace Onyx
