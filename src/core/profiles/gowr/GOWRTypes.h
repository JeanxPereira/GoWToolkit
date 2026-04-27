#pragma once
#include <cstdint>
#include <string>

// ── GOWRTypes.h ────────────────────────────────────────────────────────────
// Shared binary structures and type helpers for God of War Ragnarök WAD parsing.
// Moved here from ProfileGOWR.cpp so WadNodeBuilder.cpp can include them.
//
// Confirmed layout via Ghidra (GoWR.exe) + direct binary inspection of r_fox00.wad.
// stride = 0x90 (144 bytes) per FileDesc entry.

namespace GOW {

// ── Binary structures ──────────────────────────────────────────────────────

#pragma pack(push, 1)
struct GOWRWadHeader {
    uint32_t magic;       // 0x434F5457 = 'WTOC'
    uint32_t ver;         // 0x2
    uint32_t fileCount;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t block0Size;
    uint32_t block1Size;
    uint32_t block2Size;
    uint8_t  unk3[20];
    uint32_t block8Size;
    uint32_t unk4;
    uint32_t unk5;
};

struct GOWRFileDesc {
    uint16_t group;           // +0x00  subsystem group identifier
    uint16_t type;            // +0x02  asset type enum (see GOWRTypeToString)
    uint32_t size;            // +0x04  byte size of asset data (0 = no inline data)
    uint8_t  unk1[16];        // +0x08
    char     name[0x38];      // +0x18  null-terminated ASCII name (56 bytes max)
    uint8_t  unk2[0x1F];      // +0x50  unk2[20] != 0 → enqueue into group 8
    uint8_t  blockBitSet;     // +0x6F  streaming block group; drives offset resolution
    uint8_t  unk3[0x8];       // +0x70  unk3[2] == 1 → flush trigger
    uint32_t offset;          // +0x78  offset within block
    uint8_t  unk4[12];        // +0x7C
    uint32_t offset2;         // +0x88  secondary offset (used when blockBitSet != key)
    uint8_t  unk5[4];         // +0x8C
                              // total: 0x90 = 144 bytes
};
#pragma pack(pop)

static_assert(sizeof(GOWRFileDesc) == 0x90, "GOWRFileDesc must be 144 bytes");
static_assert(sizeof(GOWRWadHeader) == 64,  "GOWRWadHeader must be 64 bytes");

// ── Type string mapping ────────────────────────────────────────────────────
// Maps the 16-bit type field from GOWRFileDesc to a human-readable schema type string.
// Used by WadNodeBuilder to populate ParsedEntry::schemaType.

inline std::string GOWRTypeToString(uint16_t type) {
    switch (type) {
        // Mesh & Geometry
        case 0x0000: return "GOWR_MESH_MAP";
        case 0x0001: return "GOWR_MESH_DEFN";
        case 0x000B: return "GOWR_RIGID_MESH";
        case 0x000C: return "GOWR_SMSH_DEFN";
        case 0x0015: return "GOWR_MG_DEFN";
        case 0x0098: return "GOWR_SKINNED_MESH";
        case 0x0099: return "GOWR_MG_DEFN_ALT";
        case 0x8198: return "GOWR_SKINNED_MESH_BUFF";
        case 0x8199: return "GOWR_MG_GPU_BUFF";
        // Model & Instance
        case 0x008F: return "GOWR_MODEL_INSTANCE";
        case 0x0090: return "GOWR_SKIN_INSTANCE";
        // Skeleton & Rig
        case 0x003D: return "GOWR_RIG";
        case 0x003F: return "GOWR_GOPROTO_RIG";
        case 0x0040: return "GOWR_JOINT_MAP";
        // Animation
        case 0x0008: return "GOWR_ANIMATION";
        case 0x0041: return "GOWR_ANIMATION_ALT";
        case 0x0042: return "GOWR_ANM_CLIP";
        // Shader & Material
        case 0x801E: return "GOWR_SHADER";
        case 0x8020: return "GOWR_MATERIAL";
        // Texture
        case 0x80A1: return "GOWR_TEXTURE_OLD";
        case 0x80A2: return "GOWR_TEXTURE";
        // Collision & Physics
        case 0x0050: return "GOWR_COLLISION";
        case 0x0051: return "GOWR_PHYS_DATA";
        // Script & Entity
        case 0x0004: return "GOWR_SCRIPT";
        case 0x0005: return "GOWR_ENTITY";
        case 0x0006: return "GOWR_LEVEL";
        // Audio
        case 0x00C8: return "GOWR_SOUND_BANK";
        case 0x00C9: return "GOWR_SOUND_DATA";
        // Unknowns explicitly tracked
        case 0x0002: return "GOWR_UNKNOWN_2";
        case 0x0021: return "GOWR_UNKNOWN_33";
        default:     return "GOWR_UNKNOWN_" + std::to_string(type);
    }
}

} // namespace GOW
