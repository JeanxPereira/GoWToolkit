#pragma once

// â”€â”€ Umbrella header â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Provides all UI helper functions through focused sub-headers.
// Existing #include "UIHelpers.h" call sites continue to work unchanged.

// Formatting utilities (HashHex, FormatBytes, FormatNum, MatchesFilter)
#include <Onyx/App/Formatting.h>
// Bring Onyx::App:: formatting helpers into global scope for backward compatibility
using Onyx::App::HashHex;
using Onyx::App::FormatBytes;
using Onyx::App::FormatNum;
using Onyx::App::MatchesFilter;

// TypeId â†’ visual mapping (TypeName, ColorForType, IconForType, SkinModeName)
#include <Onyx/App/TypeVisuals.h>

// NOTE: Role â†’ visual mapping (ColorForRole/IconForRole) is GOWR-specific and
// lives in the app (ui/RoleVisuals.h). It is deliberately NOT pulled in here â€”
// this umbrella is engine-side and must stay game-agnostic. App TUs that need
// role visuals include "ui/RoleVisuals.h" directly.

// â”€â”€ Platform dialogs â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#include <string>

std::string SystemOpenFileDialog();
std::string SystemSaveFileDialog(const std::string& defaultName);
