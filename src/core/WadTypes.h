#pragma once

// ── Legacy umbrella header ────────────────────────────────────────────────
//
// `WadTypes.h` historically held `WadAssetName`, `WadEntryRole`,
// `WadBlock`, `ParsedEntry`, `OpenWad`, and `TypeIdToSchemaString`. M1.T1
// split each concept into its own header under `core/domain/`. This
// umbrella keeps existing call sites working (strangler-fig
// migration); new code should `#include "core/domain/<Specific>.h"`
// instead of `core/WadTypes.h`.
//
// `TypeIdToSchemaString` will be retired in M4 along with the schema
// string itself, so it deliberately stays here rather than getting a
// fresh home.

#include "core/domain/Entry.h"
#include "core/domain/Wad.h"
#include "core/domain/WadEntryRoleLegacy.h"
#include "core/types/GameVersion.h"
#include "core/types/TypeId.h"

// Bridge: converts TypeId to legacy schema string for UI display.
inline std::string TypeIdToSchemaString(GOW::GameVersion ver, GOW::TypeId id) {
    if (ver == GOW::GameVersion::GOW1) {
        switch (id) {
            case GOW::TypeId::Model:    return "GOW1_MDL";
            case GOW::TypeId::Mesh:     return "GOW1_MESH";
            case GOW::TypeId::Material: return "GOW1_MAT";
            case GOW::TypeId::Texture:  return "GOW1_TXR";
            case GOW::TypeId::Animation:return "GOW1_ANM";
            default: break;
        }
    } else if (ver == GOW::GameVersion::GOW2) {
        switch (id) {
            case GOW::TypeId::Instance: return "GOW2_SERVER_INSTANCE";
            case GOW::TypeId::Object:   return "GOW2_OBJ";
            case GOW::TypeId::Model:    return "GOW2_MDL";
            case GOW::TypeId::Mesh:     return "GOW2_MESH";
            case GOW::TypeId::Material: return "GOW2_MAT";
            case GOW::TypeId::Texture:  return "GOW2_TXR";
            case GOW::TypeId::GfxData:  return "GOW2_GFX";
            case GOW::TypeId::PalData:  return "GOW2_PAL";
            case GOW::TypeId::Animation:return "GOW2_ANM";
            case GOW::TypeId::Script:   return "GOW2_SCP";
            case GOW::TypeId::Light:    return "GOW2_LGT";
            case GOW::TypeId::Sound:    return "GOW2_SFX"; // or VAG/VPK depending on extension
            case GOW::TypeId::EntityCount: return "GOW2_ENTITY_COUNT";
            case GOW::TypeId::GroupStart:  return "GOW2_GROUP_START";
            case GOW::TypeId::GroupEnd:    return "GOW2_GROUP_END";
            case GOW::TypeId::WadFile:     return "GOW2_WAD_FILE";
            default: break;
        }
    }
    return "";
}
