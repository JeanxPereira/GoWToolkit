#pragma once
#include "Core/Types/TypeRegistry.h"
#include "Core/Types/TypeCatalog.h"
#include "Fonts/SFSymbols.h"
#include "imgui.h"

// â”€â”€ TypeId â†’ name / color / icon â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// First try the registered handler (richer, per-handler metadata); otherwise
// fall back to the runtime TypeCatalog (seeded from the game type table).

inline const char *TypeName(Onyx::Types::TypeId typeId) {
  if (auto *handler = Onyx::Types::TypeRegistry::Get().Resolve(typeId)) {
    return handler->GetName();
  }
  return Onyx::Types::TypeCatalog::Get().Label(typeId);
}

inline ImVec4 ColorForType(Onyx::Types::TypeId typeId) {
  if (auto *handler = Onyx::Types::TypeRegistry::Get().Resolve(typeId)) {
    auto c = handler->GetColor();
    return {c.r, c.g, c.b, c.a};
  }
  float c[4];
  Onyx::Types::TypeCatalog::Get().Color(typeId, c);
  return {c[0], c[1], c[2], c[3]};
}

inline const char *IconForType(Onyx::Types::TypeId typeId) {
  if (auto *handler = Onyx::Types::TypeRegistry::Get().Resolve(typeId)) {
    return handler->GetIcon();
  }
  return Onyx::Types::TypeCatalog::Get().Icon(typeId);
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
