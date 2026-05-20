#pragma once
#include "core/profiles/gowr/GowrTaxonomy.h"
#include "fonts/SFSymbols.h"
#include "imgui.h"

// ── Role-based color / icon (GOWR WAD entries) ────────────────────────────

inline ImVec4 ColorForRole(GOW::Gowr::WadEntryRole role) {
  switch (role) {
  case GOW::Gowr::WadEntryRole::ManifestBlock:
  case GOW::Gowr::WadEntryRole::WadIdentity:
    return {0.95f, 0.95f, 0.95f, 1.0f}; // near-white
  case GOW::Gowr::WadEntryRole::ShaderBlock:
  case GOW::Gowr::WadEntryRole::ShaderGroup:
  case GOW::Gowr::WadEntryRole::ShaderVertex:
  case GOW::Gowr::WadEntryRole::ShaderPixel:
    return {0.50f, 1.00f, 0.50f, 1.0f}; // green
  case GOW::Gowr::WadEntryRole::ShaderContainer:
    return {0.35f, 0.75f, 0.35f, 1.0f}; // darker green
  case GOW::Gowr::WadEntryRole::AssetBlock:
    return {0.80f, 0.80f, 1.00f, 1.0f}; // light blue
  case GOW::Gowr::WadEntryRole::ParticleBlock:
  case GOW::Gowr::WadEntryRole::FxGroup:
  case GOW::Gowr::WadEntryRole::ParticleEmitter:
  case GOW::Gowr::WadEntryRole::ParticleSystem:
    return {1.00f, 0.60f, 0.90f, 1.0f}; // lavender
  case GOW::Gowr::WadEntryRole::SharedWadRef:
    return {0.60f, 0.75f, 1.00f, 1.0f}; // periwinkle
  case GOW::Gowr::WadEntryRole::Sentinel:
    return {0.40f, 0.40f, 0.40f, 1.0f}; // dark gray
  case GOW::Gowr::WadEntryRole::AnimClip:
    return {1.00f, 0.85f, 0.30f, 1.0f}; // amber
  case GOW::Gowr::WadEntryRole::TexturePair:
  case GOW::Gowr::WadEntryRole::TextureGpu:
  case GOW::Gowr::WadEntryRole::TextureCpu:
    return {1.00f, 0.50f, 0.80f, 1.0f}; // pink
  case GOW::Gowr::WadEntryRole::Material:
  case GOW::Gowr::WadEntryRole::MaterialRef:
    return {0.95f, 0.60f, 0.20f, 1.0f}; // orange
  case GOW::Gowr::WadEntryRole::LodBinding:
    return {0.65f, 0.65f, 0.65f, 1.0f}; // gray
  case GOW::Gowr::WadEntryRole::MeshGpu:
  case GOW::Gowr::WadEntryRole::MeshDefn:
    return {0.40f, 0.80f, 1.00f, 1.0f}; // sky blue
  case GOW::Gowr::WadEntryRole::Model:
    return {0.55f, 0.90f, 1.00f, 1.0f}; // lighter sky blue
  case GOW::Gowr::WadEntryRole::GameObjectProto:
  case GOW::Gowr::WadEntryRole::GameObjectInst:
  case GOW::Gowr::WadEntryRole::GameObjectOverride:
    return {1.00f, 0.70f, 0.70f, 1.0f}; // salmon
  case GOW::Gowr::WadEntryRole::SoundEmitter:
    return {0.30f, 0.90f, 0.60f, 1.0f}; // teal
  case GOW::Gowr::WadEntryRole::ClientGuid:
    return {0.30f, 0.30f, 0.30f, 0.50f}; // very dim
  default:
    return {0.60f, 0.60f, 0.60f, 1.0f};
  }
}

inline const char *IconForRole(GOW::Gowr::WadEntryRole role) {
  switch (role) {
  case GOW::Gowr::WadEntryRole::ManifestBlock:
  case GOW::Gowr::WadEntryRole::ShaderBlock:
  case GOW::Gowr::WadEntryRole::AssetBlock:
  case GOW::Gowr::WadEntryRole::ParticleBlock:
  case GOW::Gowr::WadEntryRole::ShaderGroup:
  case GOW::Gowr::WadEntryRole::FxGroup:
  case GOW::Gowr::WadEntryRole::LodBinding:
    return ICON_SF_FOLDER; // folder
  case GOW::Gowr::WadEntryRole::SharedWadRef:
    return ICON_SF_FOLDER_FILL; // folder-opened
  case GOW::Gowr::WadEntryRole::WadIdentity:
    return ICON_SF_DOCUMENT; // file
  case GOW::Gowr::WadEntryRole::AnimClip:
    return ICON_SF_PLAY_FILL; // play
  case GOW::Gowr::WadEntryRole::TexturePair:
  case GOW::Gowr::WadEntryRole::TextureGpu:
  case GOW::Gowr::WadEntryRole::TextureCpu:
    return ICON_SF_PHOTO; // file-media
  case GOW::Gowr::WadEntryRole::Material:
  case GOW::Gowr::WadEntryRole::MaterialRef:
    return ICON_SF_PAINTPALETTE_FILL; // symbol-color
  case GOW::Gowr::WadEntryRole::MeshGpu:
  case GOW::Gowr::WadEntryRole::MeshDefn:
  case GOW::Gowr::WadEntryRole::Model:
    return ICON_SF_CUBE_FILL; // symbol-misc
  case GOW::Gowr::WadEntryRole::ShaderVertex:
  case GOW::Gowr::WadEntryRole::ShaderPixel:
  case GOW::Gowr::WadEntryRole::ShaderContainer:
    return ICON_SF_CURLYBRACES; // code
  case GOW::Gowr::WadEntryRole::GameObjectProto:
  case GOW::Gowr::WadEntryRole::GameObjectInst:
  case GOW::Gowr::WadEntryRole::GameObjectOverride:
    return ICON_SF_PERSON_FILL; // person
  case GOW::Gowr::WadEntryRole::SoundEmitter:
    return ICON_SF_SPEAKER_WAVE_2_FILL; // unmute
  case GOW::Gowr::WadEntryRole::ParticleEmitter:
  case GOW::Gowr::WadEntryRole::ParticleSystem:
    return ICON_SF_SPARKLES; // sparkle
  case GOW::Gowr::WadEntryRole::Sentinel:
  case GOW::Gowr::WadEntryRole::ClientGuid:
    return ICON_SF_MINUS; // dash
  default:
    return ICON_SF_DOCUMENT; // file
  }
}
