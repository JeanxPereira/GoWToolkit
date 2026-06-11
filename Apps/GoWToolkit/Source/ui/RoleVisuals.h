#pragma once
#include "core/profiles/gowr/GowrTaxonomy.h"
#include "fonts/SFSymbols.h"
#include "imgui.h"

// ── Role-based color / icon (GOWR WAD entries) ────────────────────────────

inline ImVec4 ColorForRole(Onyx::Gowr::WadEntryRole role) {
  switch (role) {
  case Onyx::Gowr::WadEntryRole::ManifestBlock:
  case Onyx::Gowr::WadEntryRole::WadIdentity:
    return {0.95f, 0.95f, 0.95f, 1.0f}; // near-white
  case Onyx::Gowr::WadEntryRole::ShaderBlock:
  case Onyx::Gowr::WadEntryRole::ShaderGroup:
  case Onyx::Gowr::WadEntryRole::ShaderVertex:
  case Onyx::Gowr::WadEntryRole::ShaderPixel:
    return {0.50f, 1.00f, 0.50f, 1.0f}; // green
  case Onyx::Gowr::WadEntryRole::ShaderContainer:
    return {0.35f, 0.75f, 0.35f, 1.0f}; // darker green
  case Onyx::Gowr::WadEntryRole::AssetBlock:
    return {0.80f, 0.80f, 1.00f, 1.0f}; // light blue
  case Onyx::Gowr::WadEntryRole::ParticleBlock:
  case Onyx::Gowr::WadEntryRole::FxGroup:
  case Onyx::Gowr::WadEntryRole::ParticleEmitter:
  case Onyx::Gowr::WadEntryRole::ParticleSystem:
    return {1.00f, 0.60f, 0.90f, 1.0f}; // lavender
  case Onyx::Gowr::WadEntryRole::SharedWadRef:
    return {0.60f, 0.75f, 1.00f, 1.0f}; // periwinkle
  case Onyx::Gowr::WadEntryRole::Sentinel:
    return {0.40f, 0.40f, 0.40f, 1.0f}; // dark gray
  case Onyx::Gowr::WadEntryRole::AnimClip:
    return {1.00f, 0.85f, 0.30f, 1.0f}; // amber
  case Onyx::Gowr::WadEntryRole::TexturePair:
  case Onyx::Gowr::WadEntryRole::TextureGpu:
  case Onyx::Gowr::WadEntryRole::TextureCpu:
    return {1.00f, 0.50f, 0.80f, 1.0f}; // pink
  case Onyx::Gowr::WadEntryRole::Material:
  case Onyx::Gowr::WadEntryRole::MaterialRef:
    return {0.95f, 0.60f, 0.20f, 1.0f}; // orange
  case Onyx::Gowr::WadEntryRole::LodBinding:
    return {0.65f, 0.65f, 0.65f, 1.0f}; // gray
  case Onyx::Gowr::WadEntryRole::MeshGpu:
  case Onyx::Gowr::WadEntryRole::MeshDefn:
    return {0.40f, 0.80f, 1.00f, 1.0f}; // sky blue
  case Onyx::Gowr::WadEntryRole::Model:
    return {0.55f, 0.90f, 1.00f, 1.0f}; // lighter sky blue
  case Onyx::Gowr::WadEntryRole::GameObjectProto:
  case Onyx::Gowr::WadEntryRole::GameObjectInst:
  case Onyx::Gowr::WadEntryRole::GameObjectOverride:
    return {1.00f, 0.70f, 0.70f, 1.0f}; // salmon
  case Onyx::Gowr::WadEntryRole::SoundEmitter:
    return {0.30f, 0.90f, 0.60f, 1.0f}; // teal
  case Onyx::Gowr::WadEntryRole::ClientGuid:
    return {0.30f, 0.30f, 0.30f, 0.50f}; // very dim
  default:
    return {0.60f, 0.60f, 0.60f, 1.0f};
  }
}

inline const char *IconForRole(Onyx::Gowr::WadEntryRole role) {
  switch (role) {
  case Onyx::Gowr::WadEntryRole::ManifestBlock:
  case Onyx::Gowr::WadEntryRole::ShaderBlock:
  case Onyx::Gowr::WadEntryRole::AssetBlock:
  case Onyx::Gowr::WadEntryRole::ParticleBlock:
  case Onyx::Gowr::WadEntryRole::ShaderGroup:
  case Onyx::Gowr::WadEntryRole::FxGroup:
  case Onyx::Gowr::WadEntryRole::LodBinding:
    return ICON_SF_FOLDER; // folder
  case Onyx::Gowr::WadEntryRole::SharedWadRef:
    return ICON_SF_FOLDER_FILL; // folder-opened
  case Onyx::Gowr::WadEntryRole::WadIdentity:
    return ICON_SF_DOCUMENT; // file
  case Onyx::Gowr::WadEntryRole::AnimClip:
    return ICON_SF_PLAY_FILL; // play
  case Onyx::Gowr::WadEntryRole::TexturePair:
  case Onyx::Gowr::WadEntryRole::TextureGpu:
  case Onyx::Gowr::WadEntryRole::TextureCpu:
    return ICON_SF_PHOTO; // file-media
  case Onyx::Gowr::WadEntryRole::Material:
  case Onyx::Gowr::WadEntryRole::MaterialRef:
    return ICON_SF_PAINTPALETTE_FILL; // symbol-color
  case Onyx::Gowr::WadEntryRole::MeshGpu:
  case Onyx::Gowr::WadEntryRole::MeshDefn:
  case Onyx::Gowr::WadEntryRole::Model:
    return ICON_SF_CUBE_FILL; // symbol-misc
  case Onyx::Gowr::WadEntryRole::ShaderVertex:
  case Onyx::Gowr::WadEntryRole::ShaderPixel:
  case Onyx::Gowr::WadEntryRole::ShaderContainer:
    return ICON_SF_CURLYBRACES; // code
  case Onyx::Gowr::WadEntryRole::GameObjectProto:
  case Onyx::Gowr::WadEntryRole::GameObjectInst:
  case Onyx::Gowr::WadEntryRole::GameObjectOverride:
    return ICON_SF_PERSON_FILL; // person
  case Onyx::Gowr::WadEntryRole::SoundEmitter:
    return ICON_SF_SPEAKER_WAVE_2_FILL; // unmute
  case Onyx::Gowr::WadEntryRole::ParticleEmitter:
  case Onyx::Gowr::WadEntryRole::ParticleSystem:
    return ICON_SF_SPARKLES; // sparkle
  case Onyx::Gowr::WadEntryRole::Sentinel:
  case Onyx::Gowr::WadEntryRole::ClientGuid:
    return ICON_SF_MINUS; // dash
  default:
    return ICON_SF_DOCUMENT; // file
  }
}
