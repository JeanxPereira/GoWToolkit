#pragma once
#include "core/AssetDatabase.h"
#include "fonts/SFSymbols.h"
#include "imgui.h"
#include <iomanip>
#include <sstream>
#include <string>

std::string SystemOpenFileDialog();

// ── Formatting ─────────────────────────────────────────────────────────────

inline std::string HashHex(uint64_t hash) {
  std::ostringstream ss;
  ss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(16)
     << hash;
  return ss.str();
}

inline std::string FormatBytes(uint64_t bytes) {
  if (bytes < 1024)
    return std::to_string(bytes) + " B";
  if (bytes < 1024 * 1024)
    return std::to_string(bytes / 1024) + " KB";
  return std::to_string(bytes / (1024 * 1024)) + " MB";
}

inline std::string FormatNum(uint64_t n) {
  // Insert thousands separator
  std::string s = std::to_string(n);
  for (int i = (int)s.size() - 3; i > 0; i -= 3)
    s.insert(i, ",");
  return s;
}

// ── TypeId / schemaType → name / color / icon ─────────────────────────────

#include "core/types/TypeRegistry.h"

inline const char *TypeName(GOW::GameVersion ver, GOW::TypeId typeId,
                            const std::string &schemaType) {
  auto *handler = GOW::TypeRegistry::Get().Resolve(typeId);
  if (handler) {
    return handler->GetName();
  }

  // GOWR fallbacks / unmigrated types
  if (schemaType == "GOWR_SHADER")
    return "SHADER";
  if (schemaType == "GOWR_MESH_DEFN")
    return "GOWR MESH DEFN";
  if (schemaType == "GOWR_MODL_DEFN")
    return "GOWR MDL DEFN";
  if (schemaType == "GOWR_TEXTURE")
    return "GOWR TEXTURE";
  if (schemaType == "GOWR_GOPROTO_RIG")
    return "GOWR GO PROTO RIG";
  return schemaType.empty() ? "UNKNOWN" : schemaType.c_str();
}

inline ImVec4 ColorForType(GOW::GameVersion ver, GOW::TypeId typeId,
                           const std::string &schemaType) {
  auto *handler = GOW::TypeRegistry::Get().Resolve(typeId);
  if (handler) {
    auto c = handler->GetColor();
    return {c.r, c.g, c.b, c.a};
  }

  // GOWR fallbacks
  if (schemaType == "GOWR_MESH_DEFN" || schemaType == "GOWR_MODL_DEFN")
    return {0.4f, 0.8f, 1.0f, 1.0f};
  if (schemaType == "GOWR_TEXTURE")
    return {1.0f, 0.5f, 0.8f, 1.0f};
  if (schemaType == "GOWR_SHADER")
    return {0.5f, 1.0f, 0.5f, 1.0f};
  if (schemaType == "GOWR_GOPROTO_RIG" || schemaType == "GOWR_MG_DEFN")
    return {1.0f, 0.6f, 0.3f, 1.0f};
  if (schemaType == "GOWR_MG_GPU_BUFF")
    return {0.7f, 0.7f, 1.0f, 1.0f};
  return {0.6f, 0.6f, 0.6f, 1.0f}; // gray
}

inline const char *IconForType(GOW::GameVersion ver, GOW::TypeId typeId,
                               const std::string &schemaType) {
  auto *handler = GOW::TypeRegistry::Get().Resolve(typeId);
  if (handler) {
    return handler->GetIcon();
  }

  // GOWR fallbacks
  if (schemaType == "GOWR_MESH_DEFN" || schemaType == "GOWR_MG_DEFN" ||
      schemaType == "GOWR_MODL_DEFN")
    return ICON_SF_CUBE_FILL;
  if (schemaType == "GOWR_TEXTURE")
    return ICON_SF_PHOTO;
  if (schemaType == "GOWR_SHADER")
    return ICON_SF_CURLYBRACES;
  if (schemaType == "GOWR_GOPROTO_RIG")
    return ICON_SF_PERSON_FILL;

  return ICON_SF_DOCUMENT;
}

inline const char *SkinModeName(uint8_t mode) {
  switch (mode) {
  case 1:
    return "4-8 joints (R10G10B10A2)";
  case 2:
    return "7 joints (R16)";
  case 3:
    return "10 joints (packed)";
  default:
    return "unknown";
  }
}

inline bool MatchesFilter(const std::string &name, const char *filter) {
  if (!filter || !filter[0])
    return true;
  std::string n = name, f = filter;
  for (auto &c : n)
    c = (char)tolower(c);
  for (auto &c : f)
    c = (char)tolower(c);
  return n.find(f) != std::string::npos;
}

// ── Role-based color / icon (GOWR WAD entries) ────────────────────────────

inline ImVec4 ColorForRole(WadEntryRole role) {
  switch (role) {
  case WadEntryRole::ManifestBlock:
  case WadEntryRole::WadIdentity:
    return {0.95f, 0.95f, 0.95f, 1.0f}; // near-white
  case WadEntryRole::ShaderBlock:
  case WadEntryRole::ShaderGroup:
  case WadEntryRole::ShaderVertex:
  case WadEntryRole::ShaderPixel:
    return {0.50f, 1.00f, 0.50f, 1.0f}; // green
  case WadEntryRole::ShaderContainer:
    return {0.35f, 0.75f, 0.35f, 1.0f}; // darker green
  case WadEntryRole::AssetBlock:
    return {0.80f, 0.80f, 1.00f, 1.0f}; // light blue
  case WadEntryRole::ParticleBlock:
  case WadEntryRole::FxGroup:
  case WadEntryRole::ParticleEmitter:
  case WadEntryRole::ParticleSystem:
    return {1.00f, 0.60f, 0.90f, 1.0f}; // lavender
  case WadEntryRole::SharedWadRef:
    return {0.60f, 0.75f, 1.00f, 1.0f}; // periwinkle
  case WadEntryRole::Sentinel:
    return {0.40f, 0.40f, 0.40f, 1.0f}; // dark gray
  case WadEntryRole::AnimClip:
    return {1.00f, 0.85f, 0.30f, 1.0f}; // amber
  case WadEntryRole::TexturePair:
  case WadEntryRole::TextureGpu:
  case WadEntryRole::TextureCpu:
    return {1.00f, 0.50f, 0.80f, 1.0f}; // pink
  case WadEntryRole::Material:
  case WadEntryRole::MaterialRef:
    return {0.95f, 0.60f, 0.20f, 1.0f}; // orange
  case WadEntryRole::LodBinding:
    return {0.65f, 0.65f, 0.65f, 1.0f}; // gray
  case WadEntryRole::MeshGpu:
  case WadEntryRole::MeshDefn:
    return {0.40f, 0.80f, 1.00f, 1.0f}; // sky blue
  case WadEntryRole::Model:
    return {0.55f, 0.90f, 1.00f, 1.0f}; // lighter sky blue
  case WadEntryRole::GameObjectProto:
  case WadEntryRole::GameObjectInst:
  case WadEntryRole::GameObjectOverride:
    return {1.00f, 0.70f, 0.70f, 1.0f}; // salmon
  case WadEntryRole::SoundEmitter:
    return {0.30f, 0.90f, 0.60f, 1.0f}; // teal
  case WadEntryRole::ClientGuid:
    return {0.30f, 0.30f, 0.30f, 0.50f}; // very dim
  default:
    return {0.60f, 0.60f, 0.60f, 1.0f};
  }
}

inline const char *IconForRole(WadEntryRole role) {
  switch (role) {
  case WadEntryRole::ManifestBlock:
  case WadEntryRole::ShaderBlock:
  case WadEntryRole::AssetBlock:
  case WadEntryRole::ParticleBlock:
  case WadEntryRole::ShaderGroup:
  case WadEntryRole::FxGroup:
  case WadEntryRole::LodBinding:
    return ICON_SF_FOLDER; // folder
  case WadEntryRole::SharedWadRef:
    return ICON_SF_FOLDER_FILL; // folder-opened
  case WadEntryRole::WadIdentity:
    return ICON_SF_DOCUMENT; // file
  case WadEntryRole::AnimClip:
    return ICON_SF_PLAY_FILL; // play
  case WadEntryRole::TexturePair:
  case WadEntryRole::TextureGpu:
  case WadEntryRole::TextureCpu:
    return ICON_SF_PHOTO; // file-media
  case WadEntryRole::Material:
  case WadEntryRole::MaterialRef:
    return ICON_SF_PAINTPALETTE_FILL; // symbol-color
  case WadEntryRole::MeshGpu:
  case WadEntryRole::MeshDefn:
  case WadEntryRole::Model:
    return ICON_SF_CUBE_FILL; // symbol-misc
  case WadEntryRole::ShaderVertex:
  case WadEntryRole::ShaderPixel:
  case WadEntryRole::ShaderContainer:
    return ICON_SF_CURLYBRACES; // code
  case WadEntryRole::GameObjectProto:
  case WadEntryRole::GameObjectInst:
  case WadEntryRole::GameObjectOverride:
    return ICON_SF_PERSON_FILL; // person
  case WadEntryRole::SoundEmitter:
    return ICON_SF_SPEAKER_WAVE_2_FILL; // unmute
  case WadEntryRole::ParticleEmitter:
  case WadEntryRole::ParticleSystem:
    return ICON_SF_SPARKLES; // sparkle
  case WadEntryRole::Sentinel:
  case WadEntryRole::ClientGuid:
    return ICON_SF_MINUS; // dash
  default:
    return ICON_SF_DOCUMENT; // file
  }
}
