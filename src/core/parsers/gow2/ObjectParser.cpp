// ObjectParser — GOW1/GOW2 skeleton/joints parser
// Port of god_of_war_browser/pack/wad/obj/obj.go + obj_gow2.go
//
// Magic numbers:
//   GOW1: 0x00040001 — header 0x2C bytes, joint entry at HEADER + i*0x10, name at HEADER + N*0x10 + i*0x18
//   GOW2: 0x00010001 — header 0x14 bytes, joint entry at 0x14 + i*0x10, name at 0x14 + N*0x10 + i*0x18
//
// Data section (at dataOffset): 0x30-byte header with matrix counts and offsets,
// followed by matrix arrays (Matrixes1, Matrixes2, Matrixes3) and vector arrays (Vectors4-7).

#include "ObjectParser.h"
#include "core/Logger.h"
#include <cstring>
#include <cmath>

namespace GOW {

// Constants matching god_of_war_browser/pack/wad/obj
static const uint32_t DATA_HEADER_SIZE = 0x30;
static const uint32_t GOW2_HEADER_SIZE = 0x14;
static const uint32_t MAGIC_GOW2       = 0x00010001;

// ── Helpers ────────────────────────────────────────────────────────────────

static std::string ReadFixedString(const uint8_t* buf, size_t maxLen) {
    size_t len = strnlen(reinterpret_cast<const char*>(buf), maxLen);
    return std::string(reinterpret_cast<const char*>(buf), len);
}

static uint16_t ReadU16(const uint8_t* buf) {
    uint16_t v; std::memcpy(&v, buf, 2); return v;
}

static int16_t ReadI16(const uint8_t* buf) {
    int16_t v; std::memcpy(&v, buf, 2); return v;
}

static uint32_t ReadU32(const uint8_t* buf) {
    uint32_t v; std::memcpy(&v, buf, 4); return v;
}

static float ReadF32(const uint8_t* buf) {
    float v; std::memcpy(&v, buf, 4); return v;
}

static glm::mat4 ReadMat4(const uint8_t* buf) {
    glm::mat4 m;
    // glm stores column-major, PS2 data is also column-major (row in memory = column for glm)
    // Read 16 floats directly — glm::mat4 has 16 contiguous floats
    std::memcpy(&m[0][0], buf, 64);
    return m;
}

static glm::vec4 ReadVec4(const uint8_t* buf) {
    glm::vec4 v;
    std::memcpy(&v[0], buf, 16);
    return v;
}

static glm::ivec4 ReadIVec4(const uint8_t* buf) {
    glm::ivec4 v;
    std::memcpy(&v[0], buf, 16);
    return v;
}

// ── Joint parsing (GOW2 layout) ──────────────────────────────────────────

static bool ParseJoints(const uint8_t* data, uint32_t size, ObjectData& obj) {
    constexpr uint32_t headerSize = GOW2_HEADER_SIZE;

    uint32_t jointCount = ReadU32(data + 0x04);

    if (jointCount == 0 || jointCount > 1024) {
        LOG_WARN("[ObjectParser] Suspicious joint count: %u", jointCount);
        return false;
    }

    // Data offset (where the matrix data block starts)
    uint32_t dataOffset = ReadU32(data + 0x10);

    if (dataOffset + DATA_HEADER_SIZE > size) {
        LOG_ERR("[ObjectParser] Data offset 0x%X exceeds buffer size %u", dataOffset, size);
        return false;
    }

    // ── Parse joint entries ──────────────────────────────────────────────
    obj.joints.resize(jointCount);
    int16_t invId = 0;

    for (uint32_t i = 0; i < jointCount; ++i) {
        uint32_t jointBufStart = headerSize + i * 0x10;
        uint32_t nameBufStart  = headerSize + jointCount * 0x10 + i * 0x18;

        if (jointBufStart + 0x10 > size || nameBufStart + 0x18 > size) {
            LOG_ERR("[ObjectParser] Joint %u exceeds buffer", i);
            return false;
        }

        const uint8_t* jBuf = data + jointBufStart;
        const uint8_t* nBuf = data + nameBufStart;

        uint32_t flags = ReadU32(jBuf + 0x00);

        auto& j = obj.joints[i];
        j.name        = ReadFixedString(nBuf, 0x18);
        j.flags       = flags;
        j.childsStart = ReadI16(jBuf + 0x04);
        j.childsEnd   = ReadI16(jBuf + 0x06);
        j.parent      = ReadI16(jBuf + 0x08);
        j.externalId  = ReadI16(jBuf + 0x0A);
        j.unkCoeff    = ReadF32(jBuf + 0x0C);
        j.id          = static_cast<int16_t>(i);
        j.isSkinned   = (flags & 0x80) != 0;
        j.isExternal  = (flags & 0x08) != 0;
        // Render code in obj_gow2 reference checks `joint.Flags & 0x8000` per-joint
        // to pick quaternion vs Euler decoding of vectors5 / animation samples.
        // Forcing every GOW2 joint into the Euler path turned Q.14 quaternion-looking
        // values into 100°+ Euler angles, contorting the skeleton mid-anim.
        j.isQuaternion = (flags & 0x8000) != 0;
        if (i < 12) {
            LOG_INFO("[JointFlags] j[%u] flags=0x%08X skinned=%d external=%d quat=%d name='%s'",
                     i, flags, (int)j.isSkinned, (int)j.isExternal,
                     (int)j.isQuaternion, j.name.c_str());
        }
        j.invId       = invId;

        if (j.isSkinned) {
            invId++;
        }
    }

    // ── Parse data section header ────────────────────────────────────────
    const uint8_t* matdata = data + dataOffset;

    uint32_t mat1count   = ReadU32(matdata + 0x00);
    uint32_t mat2offset  = ReadU32(matdata + 0x04);
    uint32_t mat2count   = ReadU32(matdata + 0x08);
    uint32_t mat3offset  = ReadU32(matdata + 0x0C);
    uint32_t mat3count   = ReadU32(matdata + 0x10);
    uint32_t vec4offset  = ReadU32(matdata + 0x20);
    uint32_t vec5offset  = ReadU32(matdata + 0x24);
    uint32_t vec6offset  = ReadU32(matdata + 0x28);
    uint32_t vec7offset  = ReadU32(matdata + 0x2C);

    // Validate invId count matches mat3count
    if (invId != static_cast<int16_t>(mat3count)) {
        LOG_WARN("[ObjectParser] InvId mismatch: %d vs mat3count %u", invId, mat3count);
    }

    LOG_INFO("[ObjectParser] mat1=%u mat2=%u mat3=%u joints=%u",
             mat1count, mat2count, mat3count, jointCount);

    // ── Read matrix arrays ────────────────────────────────────────────────
    auto readMatArray = [&](uint32_t offset, uint32_t count) -> std::vector<glm::mat4> {
        std::vector<glm::mat4> result(count);
        uint32_t absOffset = dataOffset + offset;
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t o = absOffset + i * 0x40;
            if (o + 0x40 <= size) {
                result[i] = ReadMat4(data + o);
            }
        }
        return result;
    };

    auto readVecArray = [&](uint32_t offset, uint32_t count) -> std::vector<glm::vec4> {
        std::vector<glm::vec4> result(count);
        uint32_t absOffset = dataOffset + offset;
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t o = absOffset + i * 0x10;
            if (o + 0x10 <= size) {
                result[i] = ReadVec4(data + o);
            }
        }
        return result;
    };

    auto readIVecArray = [&](uint32_t offset, uint32_t count) -> std::vector<glm::ivec4> {
        std::vector<glm::ivec4> result(count);
        uint32_t absOffset = dataOffset + offset;
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t o = absOffset + i * 0x10;
            if (o + 0x10 <= size) {
                result[i] = ReadIVec4(data + o);
            }
        }
        return result;
    };

    // Matrixes1 start right after DATA_HEADER_SIZE
    obj.matrixes1 = readMatArray(DATA_HEADER_SIZE, mat1count);
    obj.matrixes2 = readMatArray(mat2offset, mat2count);
    obj.matrixes3 = readMatArray(mat3offset, mat3count);
    obj.vectors4  = readVecArray(vec4offset, mat1count);
    obj.vectors5  = readIVecArray(vec5offset, mat1count);
    obj.vectors6  = readVecArray(vec6offset, mat1count);
    obj.vectors7  = readVecArray(vec7offset, mat1count);

    return true;
}

// ── FillJoints ───────────────────────────────────────────────────────────
// Port of obj.go FillJoints() — compute ParentToJoint and BindToJointMat
void GOW2ObjectParser::FillJoints(ObjectData& obj) {
    for (size_t i = 0; i < obj.joints.size(); ++i) {
        auto& j = obj.joints[i];

        // ParentToJoint = Matrixes1[joint.Id]
        if (i < obj.matrixes1.size()) {
            j.parentToJoint = obj.matrixes1[i];
        }

        // BindToJointMat = Matrixes3[joint.InvId] for skinned joints, identity otherwise
        if (j.isSkinned && j.invId >= 0 && j.invId < static_cast<int16_t>(obj.matrixes3.size())) {
            j.bindToJointMat = obj.matrixes3[j.invId];
        } else {
            j.bindToJointMat = glm::mat4(1.0f);
        }

        // Compute rest-pose world matrix by walking parent chain
        // renderMat = parent.renderMat * parentToJoint
        if (j.parent >= 0 && j.parent < static_cast<int16_t>(obj.joints.size())) {
            j.renderMat = obj.joints[j.parent].renderMat * j.parentToJoint;
        } else {
            j.renderMat = j.parentToJoint;
        }
    }
}

// ── Public API ───────────────────────────────────────────────────────────

std::unique_ptr<ObjectData> GOW2ObjectParser::Parse(
    const uint8_t* data, uint32_t size, uint32_t magic)
{
    if (!data || size < 0x18) return nullptr;

    if (magic == MAGIC_GOW2) {
        return ParseGOW2(data, size);
    }

    LOG_ERR("[ObjectParser] Unknown magic: 0x%08X", magic);
    return nullptr;
}

std::unique_ptr<ObjectData> GOW2ObjectParser::ParseGOW2(const uint8_t* data, uint32_t size) {
    if (size < GOW2_HEADER_SIZE) return nullptr;

    auto obj = std::make_unique<ObjectData>();
    if (!ParseJoints(data, size, *obj)) {
        return nullptr;
    }

    FillJoints(*obj);

    LOG_INFO("[ObjectParser] GOW2: Parsed %zu joints, %zu skinned",
             obj->joints.size(),
             std::count_if(obj->joints.begin(), obj->joints.end(),
                           [](const Joint& j) { return j.isSkinned; }));
    return obj;
}

} // namespace GOW
