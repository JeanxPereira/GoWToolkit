#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Onyx::Parsers {

// ── Animation data types (from anm.go) ─────────────────────────────────────
// These identify what each state descriptor animates.
enum AnimDataTypeId : uint16_t {
    ANIM_DATATYPE_SKINNING     = 0,   // Joint transforms (rotation + position)
    ANIM_DATATYPE_MATERIAL     = 3,   // Material color or light
    ANIM_DATATYPE_UNKNOWN5     = 5,   // Show/hide, mesh switch, cameras
    ANIM_DATATYPE_TEXTUREPOS   = 8,   // UV offset animation
    ANIM_DATATYPE_TEXTURESHEET = 9,   // Texture atlas frame index
    ANIM_DATATYPE_PARTICLES    = 10,  // Particle animation
    ANIM_DATATYPE_UNKNOWN11    = 11,  // Sound emitter related
    ANIM_DATATYPE_UNKNOWN12    = 12,  // Cloth/flag simulation
};

// ── Low-level structures matching Go's raw.go ──────────────────────────────

struct AnimDataType {
    uint16_t typeId = 0;
    uint8_t  param1 = 0;
    uint8_t  param2 = 0;
};

struct AnimSamplesManager {
    uint16_t count        = 0;  // Number of samples
    uint16_t offset       = 0;  // Start frame offset
    uint16_t datasCount3  = 0;  // Extra sample count for scaling
    uint16_t offsetToData = 0;  // Low 2 bytes of 3-byte offset
};

struct DataBitMap {
    uint8_t  pairedElementsCount = 0;
    uint16_t dataOffset          = 0;
    std::vector<uint16_t> bitmap;
};

// ── Stream: holds decoded float samples per joint coordinate ────────────────

struct AnimSubstream {
    AnimSamplesManager manager;
    // Key: jointId*4 + coordIdx (0..3 for x/y/z/w)
    // Value: vector of float samples (one per frame)
    std::map<int, std::vector<float>> samples;
    bool isAdditive = false;  // true if samples[-100] sentinel present
};

// ── State descriptor header ────────────────────────────────────────────────

struct AnimStateDescrHeader {
    uint16_t baseTargetDataIndex = 0;
    uint8_t  flagsProbably       = 0;
    uint8_t  howMany64kbSkip     = 0;  // High byte of 3-byte offset
};

// ── Per-skinning-state data (mirrors JS stateDesc.Data[i]) ────────────────
// Each entry is one AnimState0Skinning — an independent rotation+position
// pair with its own managers and sub-streams.

struct SkinningState {
    AnimStateDescrHeader header;

    AnimSubstream rotationStream;
    std::vector<AnimSubstream> rotationSubStreamsAdd;
    std::vector<AnimSubstream> rotationSubStreamsRough;

    AnimSubstream positionStream;
    std::vector<AnimSubstream> positionSubStreamsAdd;
    std::vector<AnimSubstream> positionSubStreamsRough;
};

// ── Per-act state descriptor (one per data type in the animation) ──────────

struct AnimStateDescr {
    AnimStateDescrHeader header;
    uint16_t countOfSomething = 0;
    float    frameTime        = 0.0f;

    // ── NEW: array of skinning states matching JS stateDesc.Data[] ──
    // The player iterates each SkinningState independently.
    std::vector<SkinningState> skinningStates;

    // Texture position data (DATATYPE_TEXTUREPOS = 8)
    // (deferred for future pass)
};

// ── Animation act (a named clip within a group) ────────────────────────────

struct AnimAct {
    std::string name;
    float       unkFloat0x4 = 0.0f;
    float       unkFloat0xc = 0.0f;
    float       duration    = 0.0f;
    std::vector<AnimStateDescr> stateDescrs;
};

// ── Animation group (a collection of acts) ─────────────────────────────────

struct AnimGroup {
    std::string name;
    bool        isExternal = false;  // When true, acts not in this file
    std::vector<AnimAct> acts;
};

// ── Top-level parsed animation data ────────────────────────────────────────

struct AnimationData {
    uint32_t rawFlags = 0;

    struct {
        bool autoplay          = false;  // flags & 0x1
        bool jointRotAnimated  = false;  // flags & 0x1000
        bool jointPosAnimated  = false;  // flags & 0x2000
        bool jointScaleAnimated = false; // flags & 0x4000
    } parsedFlags;

    std::vector<AnimDataType> dataTypes;
    std::vector<AnimGroup>    groups;

    // Convenience: find the skinning data type index (-1 if none)
    int FindSkinningTypeIndex() const {
        for (size_t i = 0; i < dataTypes.size(); ++i) {
            if (dataTypes[i].typeId == ANIM_DATATYPE_SKINNING)
                return (int)i;
        }
        return -1;
    }

    // Count total playable acts across all groups
    int TotalActs() const {
        int count = 0;
        for (const auto& g : groups) count += (int)g.acts.size();
        return count;
    }
};

} // namespace Onyx::Parsers

// Backwards-compat aliases
namespace Onyx {
    using AnimationData         = Parsers::AnimationData;
    using AnimDataType          = Parsers::AnimDataType;
    using AnimSamplesManager    = Parsers::AnimSamplesManager;
    using DataBitMap            = Parsers::DataBitMap;
    using AnimSubstream         = Parsers::AnimSubstream;
    using AnimStateDescrHeader  = Parsers::AnimStateDescrHeader;
    using SkinningState         = Parsers::SkinningState;
    using AnimStateDescr        = Parsers::AnimStateDescr;
    using AnimAct               = Parsers::AnimAct;
    using AnimGroup             = Parsers::AnimGroup;
    using AnimDataTypeId        = Parsers::AnimDataTypeId;
    // Re-export unscoped enum constants into Onyx namespace
    using Parsers::ANIM_DATATYPE_SKINNING;
    using Parsers::ANIM_DATATYPE_MATERIAL;
    using Parsers::ANIM_DATATYPE_UNKNOWN5;
    using Parsers::ANIM_DATATYPE_TEXTUREPOS;
    using Parsers::ANIM_DATATYPE_TEXTURESHEET;
    using Parsers::ANIM_DATATYPE_PARTICLES;
    using Parsers::ANIM_DATATYPE_UNKNOWN11;
    using Parsers::ANIM_DATATYPE_UNKNOWN12;
} // namespace Onyx
