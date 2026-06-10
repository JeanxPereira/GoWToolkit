#pragma once
#include "core/types/TypeId.h"        // still the enum at this point
#include "core/domain/MediaKind.h"
#include "fonts/SFSymbols.h"
#include <cstdint>

namespace Onyx {

// One row per asset type. `legacyValue` MUST equal the type's position in the
// old `enum class TypeId` (Unknown=0, EntityCount=1, ...). This locks GTKC V9
// persistence stability: handles registered in this order get .value ==
// legacyValue, so saved visibility overrides keep round-tripping.
struct GameTypeRow {
    uint32_t     legacyValue;   // == old enum numeric value
    const char*  key;           // stable string id, e.g. "GOW2_MESH"
    const char*  label;         // == old TypeIdName()
    MediaKind    media;         // == old KindOf()
    const char*  icon;          // == old IconForType() (or ICON_SF_DOCUMENT)
    float        color[4];      // == old ColorForType() (or {.6,.6,.6,1})
};

// Order MUST match enum declaration order in TypeId.h. Do NOT include COUNT.
// `label` mirrors TypeRegistry.cpp's TypeIdName switch; `media` mirrors
// MediaKind.h's KindOf switch; `icon`/`color` mirror TypeVisuals.h's fallback
// switches (default ICON_SF_DOCUMENT / {0.6,0.6,0.6,1.0} where no distinct one).
inline constexpr GameTypeRow kGameTypeTable[] = {
    {  0, "UNKNOWN",             "Unknown",          MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  1, "ENTITY_COUNT",        "Entity Count",     MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  2, "GROUP_START",         "Group Start",      MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  3, "GROUP_END",           "Group End",        MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  4, "HEADER_START",        "Header Start",     MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  5, "HEADER_POP",          "Header Pop",       MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  6, "GOW2_INSTANCE",       "Instance",         MediaKind::Map,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  7, "GOW2_OBJECT",         "Object",           MediaKind::Skeleton,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  8, "GOW2_MODEL",          "Model",            MediaKind::Mesh,      ICON_SF_CUBE_FILL,   {0.4f, 0.8f, 1.0f, 1.0f} },
    {  9, "GOW2_MESH",           "Mesh",             MediaKind::Mesh,      ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 10, "GOW2_MATERIAL",       "Material",         MediaKind::Material,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 11, "GOW2_TEXTURE",        "Texture",          MediaKind::Image,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 12, "GOW2_GFX_DATA",       "GFX Data",         MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 13, "GOW2_PAL_DATA",       "Palette",          MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 14, "GOW2_ANIMATION",      "Animation",        MediaKind::Animation, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 15, "GOW2_SCRIPT",         "Script",           MediaKind::Script,    ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 16, "GOW2_LIGHT",          "Light",            MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 17, "GOW2_SOUND",          "Sound",            MediaKind::Audio,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 18, "GOW2_COLLISION",      "Collision",        MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 19, "GOW2_FLIPBOOK",       "Flipbook",         MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 20, "GOW2_CHUNK",          "Chunk",            MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 21, "WAD_FILE",            "WAD File",         MediaKind::Container, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 22, "VAG_AUDIO",           "VAG Audio",        MediaKind::Audio,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 23, "VPK_VIDEO",           "VPK Video",        MediaKind::Video,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 24, "PSS_VIDEO",           "PSS Video",        MediaKind::Video,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 25, "PSW_VIDEO",           "PSW Video",        MediaKind::Video,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 26, "TEXT_PLAIN",          "Text",             MediaKind::Script,    ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 27, "SHADER_CONTAINER",    "Shader",           MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 28, "SHADER_VERTEX",       "Vertex Shader",    MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 29, "SHADER_PIXEL",        "Pixel Shader",     MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 30, "SHADER_HULL",         "Hull Shader",      MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 31, "SHADER_DOMAIN",       "Domain Shader",    MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 32, "SHADER_COMPUTE",      "Compute Shader",   MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 33, "SHADER_LIBRARY",      "Library Shader",   MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 34, "GOWR_MESH_GPU",       "Mesh GPU",         MediaKind::Mesh,      ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 35, "GOWR_MESH_DEFN",      "Mesh Definition",  MediaKind::Mesh,      ICON_SF_CUBE_FILL,   {0.4f, 0.8f, 1.0f, 1.0f} },
    { 36, "GOWR_GO_PROTO",       "GO Proto",         MediaKind::Skeleton,  ICON_SF_PERSON_FILL, {1.0f, 0.6f, 0.3f, 1.0f} },
    { 37, "GOWR_GO_INSTANCE",    "GO Instance",      MediaKind::Skeleton,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 38, "GOWR_GO_OVERRIDE",    "GO Override",      MediaKind::Skeleton,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 39, "GOWR_TEXTURE_PAIR",   "Texture Pair",     MediaKind::Image,     ICON_SF_PHOTO,       {1.0f, 0.5f, 0.8f, 1.0f} },
    { 40, "GOWR_MATERIAL_REF",   "Material Ref",     MediaKind::Material,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 41, "GOWR_LOD_BINDING",    "LOD Binding",      MediaKind::Mesh,      ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 42, "GOWR_ANIM_CLIP",      "Anim Clip",        MediaKind::Animation, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 43, "GOWR_SOUND_EMITTER",  "Sound Emitter",    MediaKind::Audio,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 44, "GOWR_PARTICLE_EMITTER","Particle Emitter",MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 45, "GOWR_PARTICLE_SYSTEM","Particle System",  MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 46, "GOWR_CLIENT_GUID",    "Client GUID",      MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 47, "GOWR_WAD_IDENTITY",   "WAD Identity",     MediaKind::Container, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 48, "GOWR_SHARED_WAD_REF", "Shared WAD Ref",   MediaKind::Container, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 49, "SENTINEL",            "Sentinel",         MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
};

} // namespace Onyx
