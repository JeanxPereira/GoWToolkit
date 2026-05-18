// InstanceParser — GOW1/GOW2 game object instance transform parser.
//
// Port of god_of_war_browser/pack/wad/inst:
//   GOW1 (size 0x5C): Position1 + Euler Rotation + Scale → TRS matrix
//   GOW2 (size 0x68): 3×3 orientation columns + Vec3 position → 4×4 matrix

#include "InstanceParser.h"
#include "core/Logger.h"
#include <cstring>
#include <cmath>

namespace GOW {

// ── Helpers ────────────────────────────────────────────────────────────────

static float ReadF32(const uint8_t* buf) {
    float v; std::memcpy(&v, buf, 4); return v;
}

// Build a rotation matrix from Euler angles in radians.
// GOW uses XYZ rotation order (same as Go's utils.EulerToQuat).
// The Go code converts: euler * (180/PI) into degrees, then EulerToQuat.
// Here we work in radians directly since we build the matrix ourselves.
//
// Note: The Go source reads Rotation.xyz as radians and multiplies by (180/PI)
// before passing to EulerToQuat, which internally converts degrees→radians.
// So effectively the values ARE in radians. We just use them directly.
static void BuildTRSMatrix(float out[16],
                           float tx, float ty, float tz,
                           float rx, float ry, float rz,
                           float scale)
{
    // Rotation matrices for each axis
    float cx = std::cos(rx), sx = std::sin(rx);
    float cy = std::cos(ry), sy = std::sin(ry);
    float cz = std::cos(rz), sz = std::sin(rz);

    // Combined rotation R = Rz * Ry * Rx (column-major for OpenGL)
    // With uniform scale applied
    float s = scale;

    // Column 0
    out[0]  = s * (cy * cz);
    out[1]  = s * (cy * sz);
    out[2]  = s * (-sy);
    out[3]  = 0.0f;

    // Column 1
    out[4]  = s * (sx * sy * cz - cx * sz);
    out[5]  = s * (sx * sy * sz + cx * cz);
    out[6]  = s * (sx * cy);
    out[7]  = 0.0f;

    // Column 2
    out[8]  = s * (cx * sy * cz + sx * sz);
    out[9]  = s * (cx * sy * sz - sx * cz);
    out[10] = s * (cx * cy);
    out[11] = 0.0f;

    // Column 3: translation
    out[12] = tx;
    out[13] = ty;
    out[14] = tz;
    out[15] = 1.0f;
}

// ── Detect sky by instance name ────────────────────────────────────────────

static bool IsSkyInstance(const std::string& name) {
    // Case-insensitive check for "sky" anywhere in the name
    std::string lower = name;
    for (auto& c : lower) c = (char)std::tolower((unsigned char)c);
    return lower.find("sky") != std::string::npos;
}

// ── Parser ─────────────────────────────────────────────────────────────────

std::shared_ptr<InstanceData> GOW2InstanceParser::Parse(const ParsedEntry& entry, std::shared_ptr<IFile> parentFile) {
    if (!parentFile || entry.size < 0x60) return nullptr;

    auto data = std::make_shared<InstanceData>();

    // Read the entire buffer for this instance
    std::vector<uint8_t> buf(entry.size);
    parentFile->Seek(entry.offset, SEEK_SET);
    if (parentFile->Read(buf.data(), entry.size) != entry.size) return nullptr;

    // Sky detection from entry name
    data->isSky = IsSkyInstance(entry.name);

    // ── GOW2 Instance (size 0x68) ──────────────────────────────────────
    // Layout (from god_of_war_browser/pack/wad/inst/gow2.go):
    //   [0x00:0x04] magic (0x00030001)
    //   [0x04:0x1C] unused (GOW2 has no inline object name)
    //   [0x1C:0x1E] Id  (uint16)
    //   [0x1E:0x20] Params (uint16)
    //   [0x20:0x30] UnkVec1 (Vec4) — orientation col 0
    //   [0x30:0x40] UnkVec2 (Vec4) — orientation col 1
    //   [0x40:0x50] UnkVec3 (Vec4) — orientation col 2
    //   [0x50:0x5C] Position (Vec3) — 12 bytes
    //   [0x5C:0x68] Unk (3 × uint32)
    {
        data->objectName = ""; // GOW2: resolved via child tree

        std::memcpy(&data->id, &buf[0x1C], 2);
        std::memcpy(&data->params, &buf[0x1E], 2);

        // GOW2 instance layout (from god_of_war_browser/pack/wad/inst/gow2.go):
        //   [0x20..0x50] = UnkVec1, UnkVec2, UnkVec3 — NOT orientation! Never used for rendering.
        //   [0x50..0x5C] = Position (Vec3) — the only transform data used.
        //
        // The Go reference NEVER builds a rotation matrix from UnkVec1/2/3.
        // Using them as a 3x3 orientation matrix produces a corrupt transform
        // that collapses Y/Z to 0, making the model invisible.
        //
        // Build transform as: identity rotation + Position translation.

        // Identity rotation (3x3)
        data->transformMatrix[0]  = 1.0f;
        data->transformMatrix[1]  = 0.0f;
        data->transformMatrix[2]  = 0.0f;
        data->transformMatrix[3]  = 0.0f;

        data->transformMatrix[4]  = 0.0f;
        data->transformMatrix[5]  = 1.0f;
        data->transformMatrix[6]  = 0.0f;
        data->transformMatrix[7]  = 0.0f;

        data->transformMatrix[8]  = 0.0f;
        data->transformMatrix[9]  = 0.0f;
        data->transformMatrix[10] = 1.0f;
        data->transformMatrix[11] = 0.0f;

        // Translation from Position at [0x50:0x5C]
        data->transformMatrix[12] = ReadF32(&buf[0x50]);
        data->transformMatrix[13] = ReadF32(&buf[0x54]);
        data->transformMatrix[14] = ReadF32(&buf[0x58]);
        data->transformMatrix[15] = 1.0f;

        LOG_INFO("[InstanceParser] GOW2 instance '%s': pos=(%.2f,%.2f,%.2f) isSky=%d",
                 entry.name.c_str(),
                 data->transformMatrix[12], data->transformMatrix[13], data->transformMatrix[14],
                 data->isSky);
    }

    return data;
}

} // namespace GOW
