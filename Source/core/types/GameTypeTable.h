#pragma once
#include <Onyx/Types/TypeId.h>        // still the enum at this point
#include <Onyx/Domain/MediaKind.h>
#include <Onyx/Fonts/SFSymbols.h>
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
    Domain::MediaKind    media;         // == old KindOf()
    const char*  icon;          // == old IconForType() (or ICON_SF_DOCUMENT)
    float        color[4];      // == old ColorForType() (or {.6,.6,.6,1})
};

// Order MUST match enum declaration order in TypeId.h. Do NOT include COUNT.
// `label` mirrors TypeRegistry.cpp's TypeIdName switch; `media` mirrors
// Domain::MediaKind.h's KindOf switch; `icon`/`color` mirror TypeVisuals.h's fallback
// switches (default ICON_SF_DOCUMENT / {0.6,0.6,0.6,1.0} where no distinct one).
inline constexpr GameTypeRow kGameTypeTable[] = {
    {  0, "UNKNOWN",             "Unknown",          Domain::MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  1, "ENTITY_COUNT",        "Entity Count",     Domain::MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  2, "GROUP_START",         "Group Start",      Domain::MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  3, "GROUP_END",           "Group End",        Domain::MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  4, "HEADER_START",        "Header Start",     Domain::MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  5, "HEADER_POP",          "Header Pop",       Domain::MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  6, "GOW2_INSTANCE",       "Instance",         Domain::MediaKind::Map,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  7, "GOW2_OBJECT",         "Object",           Domain::MediaKind::Skeleton,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    {  8, "GOW2_MODEL",          "Model",            Domain::MediaKind::Mesh,      ICON_SF_CUBE_FILL,   {0.4f, 0.8f, 1.0f, 1.0f} },
    {  9, "GOW2_MESH",           "Mesh",             Domain::MediaKind::Mesh,      ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 10, "GOW2_MATERIAL",       "Material",         Domain::MediaKind::Material,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 11, "GOW2_TEXTURE",        "Texture",          Domain::MediaKind::Image,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 12, "GOW2_GFX_DATA",       "GFX Data",         Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 13, "GOW2_PAL_DATA",       "Palette",          Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 14, "GOW2_ANIMATION",      "Animation",        Domain::MediaKind::Animation, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 15, "GOW2_SCRIPT",         "Script",           Domain::MediaKind::Script,    ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 16, "GOW2_LIGHT",          "Light",            Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 17, "GOW2_SOUND",          "Sound",            Domain::MediaKind::Audio,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 18, "GOW2_COLLISION",      "Collision",        Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 19, "GOW2_FLIPBOOK",       "Flipbook",         Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 20, "GOW2_CHUNK",          "Chunk",            Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 21, "WAD_FILE",            "WAD File",         Domain::MediaKind::Container, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 22, "VAG_AUDIO",           "VAG Audio",        Domain::MediaKind::Audio,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 23, "VPK_VIDEO",           "VPK Video",        Domain::MediaKind::Video,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 24, "PSS_VIDEO",           "PSS Video",        Domain::MediaKind::Video,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 25, "PSW_VIDEO",           "PSW Video",        Domain::MediaKind::Video,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 26, "TEXT_PLAIN",          "Text",             Domain::MediaKind::Script,    ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 27, "SHADER_CONTAINER",    "Shader",           Domain::MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 28, "SHADER_VERTEX",       "Vertex Shader",    Domain::MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 29, "SHADER_PIXEL",        "Pixel Shader",     Domain::MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 30, "SHADER_HULL",         "Hull Shader",      Domain::MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 31, "SHADER_DOMAIN",       "Domain Shader",    Domain::MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 32, "SHADER_COMPUTE",      "Compute Shader",   Domain::MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 33, "SHADER_LIBRARY",      "Library Shader",   Domain::MediaKind::Shader,    ICON_SF_CURLYBRACES, {0.5f, 1.0f, 0.5f, 1.0f} },
    { 34, "GOWR_MESH_GPU",       "Mesh GPU",         Domain::MediaKind::Mesh,      ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 35, "GOWR_MESH_DEFN",      "Mesh Definition",  Domain::MediaKind::Mesh,      ICON_SF_CUBE_FILL,   {0.4f, 0.8f, 1.0f, 1.0f} },
    { 36, "GOWR_GO_PROTO",       "GO Proto",         Domain::MediaKind::Skeleton,  ICON_SF_PERSON_FILL, {1.0f, 0.6f, 0.3f, 1.0f} },
    { 37, "GOWR_GO_INSTANCE",    "GO Instance",      Domain::MediaKind::Skeleton,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 38, "GOWR_GO_OVERRIDE",    "GO Override",      Domain::MediaKind::Skeleton,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 39, "GOWR_TEXTURE_PAIR",   "Texture Pair",     Domain::MediaKind::Image,     ICON_SF_PHOTO,       {1.0f, 0.5f, 0.8f, 1.0f} },
    { 40, "GOWR_MATERIAL_REF",   "Material Ref",     Domain::MediaKind::Material,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 41, "GOWR_LOD_BINDING",    "LOD Binding",      Domain::MediaKind::Mesh,      ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 42, "GOWR_ANIM_CLIP",      "Anim Clip",        Domain::MediaKind::Animation, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 43, "GOWR_SOUND_EMITTER",  "Sound Emitter",    Domain::MediaKind::Audio,     ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 44, "GOWR_PARTICLE_EMITTER","Particle Emitter",Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 45, "GOWR_PARTICLE_SYSTEM","Particle System",  Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 46, "GOWR_CLIENT_GUID",    "Client GUID",      Domain::MediaKind::Raw,       ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 47, "GOWR_WAD_IDENTITY",   "WAD Identity",     Domain::MediaKind::Container, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 48, "GOWR_SHARED_WAD_REF", "Shared WAD Ref",   Domain::MediaKind::Container, ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 49, "SENTINEL",            "Sentinel",         Domain::MediaKind::Unknown,   ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
    { 50, "GOWR_MATERIAL",      "Material",          Domain::MediaKind::Material,  ICON_SF_DOCUMENT,    {0.6f, 0.6f, 0.6f, 1.0f} },
};

} // namespace Onyx
