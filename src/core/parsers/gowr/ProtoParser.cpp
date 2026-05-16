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

    // ── Local transforms (3x3 rot + vec3 pos), 64 bytes per bone ─────────────
    obj->matrixes1.resize(boneCount);
    obj->matrixes3.resize(boneCount);
    obj->vectors4.resize(boneCount, glm::vec4(0));
    obj->vectors5.resize(boneCount, glm::ivec4(0));
    obj->vectors6.resize(boneCount, glm::vec4(1.0f));

    std::vector<glm::mat4> local(boneCount, glm::mat4(1.0f));

    for (int j = 0; j < boneCount; ++j) {
        float m00, m01, m02, _r0;
        float m10, m11, m12, _r1;
        float m20, m21, m22, _r2;
        float px, py, pz, _p3;
        file->Read(&m00, 4); file->Read(&m01, 4); file->Read(&m02, 4); file->Read(&_r0, 4);
        file->Read(&m10, 4); file->Read(&m11, 4); file->Read(&m12, 4); file->Read(&_r1, 4);
        file->Read(&m20, 4); file->Read(&m21, 4); file->Read(&m22, 4); file->Read(&_r2, 4);
        file->Read(&px,  4); file->Read(&py,  4); file->Read(&pz,  4); file->Read(&_p3, 4);

        // Proto stores rotation with basis vectors as ROWS (DirectX/Maya style).
        // glm::mat4 is column-major with basis vectors as columns, so transpose
        // the 3x3 into glm by mapping stored[row][col] → M[col][row].
        glm::mat4 M(1.0f);
        M[0][0] = m00; M[0][1] = m10; M[0][2] = m20;
        M[1][0] = m01; M[1][1] = m11; M[1][2] = m21;
        M[2][0] = m02; M[2][1] = m12; M[2][2] = m22;
        M[3][0] = px;  M[3][1] = py;  M[3][2] = pz;

        local[j]            = M;
        obj->matrixes1[j]   = M;          // parent→joint
        obj->vectors4[j]    = glm::vec4(px, py, pz, 0);
        obj->joints[j].parentToJoint = M;
    }

    // ── World rest pose from IBM table (mesh's actual bind pose) ────────────
    // The proto file has a SECOND 4x4 table right after the "matrix" table
    // (matrices_off + N*64). For skinned bones this is the inverse bind matrix
    // used by the mesh; for non-skinned bones it's identity. Composing locals
    // gives a different default pose (e.g. T-pose) that doesn't match the
    // authored mesh, so we prefer IBM-derived bind pose when available.
    std::vector<glm::mat4> composedWorld(boneCount, glm::mat4(1.0f));
    for (int j = 0; j < boneCount; ++j) {
        int p = obj->joints[j].parent;
        composedWorld[j] = (p < 0 || p >= boneCount)
            ? local[j] : (composedWorld[p] * local[j]);
    }

    for (int j = 0; j < boneCount; ++j) {
        // Read IBM (4 rows of 4 floats, row-major). Transpose to glm.
        float a00,a01,a02,a03, a10,a11,a12,a13, a20,a21,a22,a23, a30,a31,a32,a33;
        file->Read(&a00, 4); file->Read(&a01, 4); file->Read(&a02, 4); file->Read(&a03, 4);
        file->Read(&a10, 4); file->Read(&a11, 4); file->Read(&a12, 4); file->Read(&a13, 4);
        file->Read(&a20, 4); file->Read(&a21, 4); file->Read(&a22, 4); file->Read(&a23, 4);
        file->Read(&a30, 4); file->Read(&a31, 4); file->Read(&a32, 4); file->Read(&a33, 4);

        glm::mat4 ibm(1.0f);
        ibm[0][0] = a00; ibm[0][1] = a10; ibm[0][2] = a20;
        ibm[1][0] = a01; ibm[1][1] = a11; ibm[1][2] = a21;
        ibm[2][0] = a02; ibm[2][1] = a12; ibm[2][2] = a22;
        ibm[3][0] = a30; ibm[3][1] = a31; ibm[3][2] = a32;

        // Identity IBM = bone is non-skinned; fall back to composed pose.
        bool ibmIdentity = (a00 > 0.999f && a11 > 0.999f && a22 > 0.999f &&
                            std::abs(a30) < 1e-6f && std::abs(a31) < 1e-6f && std::abs(a32) < 1e-6f);

        glm::mat4 worldMat = ibmIdentity ? composedWorld[j] : glm::inverse(ibm);
        glm::mat4 bindToJoint = ibmIdentity ? glm::inverse(composedWorld[j]) : ibm;

        obj->joints[j].renderMat      = worldMat;
        obj->joints[j].bindToJointMat = bindToJoint;
        obj->matrixes3[j]             = bindToJoint;
    }

    return obj;
}

} // namespace GOW
