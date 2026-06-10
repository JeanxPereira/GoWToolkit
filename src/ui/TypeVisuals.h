#pragma once
#include "core/types/TypeRegistry.h"
#include "fonts/SFSymbols.h"
#include "imgui.h"

// ── TypeId → name / color / icon ──────────────────────────────────────────

inline const char *TypeName(Onyx::TypeId typeId) {
  auto *handler = Onyx::TypeRegistry::Get().Resolve(typeId);
  if (handler) {
    return handler->GetName();
  }

  // GOWR fallbacks / unmigrated types
  switch (typeId) {
      case Onyx::TypeId::ShaderContainer:
      case Onyx::TypeId::ShaderVertex:
      case Onyx::TypeId::ShaderPixel:
      case Onyx::TypeId::ShaderHull:
      case Onyx::TypeId::ShaderDomain:
      case Onyx::TypeId::ShaderCompute:
      case Onyx::TypeId::ShaderLibrary:  return "SHADER";
      case Onyx::TypeId::MeshDefn:       return "GOWR MESH DEFN";
      case Onyx::TypeId::Model:          return "GOWR MDL DEFN";
      case Onyx::TypeId::TexturePair:    return "GOWR TEXTURE";
      case Onyx::TypeId::GameObjectProto:return "GOWR GO PROTO RIG";
      default:                          return "UNKNOWN";
  }
}

inline ImVec4 ColorForType(Onyx::TypeId typeId) {
  auto *handler = Onyx::TypeRegistry::Get().Resolve(typeId);
  if (handler) {
    auto c = handler->GetColor();
    return {c.r, c.g, c.b, c.a};
  }

  // GOWR fallbacks
  switch (typeId) {
      case Onyx::TypeId::MeshDefn:
      case Onyx::TypeId::Model:
          return {0.4f, 0.8f, 1.0f, 1.0f};
      case Onyx::TypeId::TexturePair:
          return {1.0f, 0.5f, 0.8f, 1.0f};
      case Onyx::TypeId::ShaderContainer:
      case Onyx::TypeId::ShaderVertex:
      case Onyx::TypeId::ShaderPixel:
      case Onyx::TypeId::ShaderHull:
      case Onyx::TypeId::ShaderDomain:
      case Onyx::TypeId::ShaderCompute:
      case Onyx::TypeId::ShaderLibrary:
          return {0.5f, 1.0f, 0.5f, 1.0f};
      case Onyx::TypeId::GameObjectProto:
          return {1.0f, 0.6f, 0.3f, 1.0f};
      // GOWR_MG_GPU_BUFF is MeshGpu role, TypeId could be Unknown or MeshGpu? 
      // We will fallback to gray.
      default:
          return {0.6f, 0.6f, 0.6f, 1.0f}; // gray
  }
}

inline const char *IconForType(Onyx::TypeId typeId) {
  auto *handler = Onyx::TypeRegistry::Get().Resolve(typeId);
  if (handler) {
    return handler->GetIcon();
  }

  // GOWR fallbacks
  switch (typeId) {
      case Onyx::TypeId::MeshDefn:
      case Onyx::TypeId::Model:
          return ICON_SF_CUBE_FILL;
      case Onyx::TypeId::TexturePair:
          return ICON_SF_PHOTO;
      case Onyx::TypeId::ShaderContainer:
      case Onyx::TypeId::ShaderVertex:
      case Onyx::TypeId::ShaderPixel:
      case Onyx::TypeId::ShaderHull:
      case Onyx::TypeId::ShaderDomain:
      case Onyx::TypeId::ShaderCompute:
      case Onyx::TypeId::ShaderLibrary:
          return ICON_SF_CURLYBRACES;
      case Onyx::TypeId::GameObjectProto:
          return ICON_SF_PERSON_FILL;
      default:
          return ICON_SF_DOCUMENT;
  }
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
