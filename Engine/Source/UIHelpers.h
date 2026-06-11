#pragma once

// ── Umbrella header ───────────────────────────────────────────────────────
// Provides all UI helper functions through focused sub-headers.
// Existing #include "UIHelpers.h" call sites continue to work unchanged.

// Formatting utilities (HashHex, FormatBytes, FormatNum, MatchesFilter)
#include "ui/Formatting.h"
// Bring Onyx:: formatting helpers into global scope for backward compatibility
using Onyx::HashHex;
using Onyx::FormatBytes;
using Onyx::FormatNum;
using Onyx::MatchesFilter;

// TypeId → visual mapping (TypeName, ColorForType, IconForType, SkinModeName)
#include "ui/TypeVisuals.h"

// NOTE: Role → visual mapping (ColorForRole/IconForRole) is GOWR-specific and
// lives in the app (ui/RoleVisuals.h). It is deliberately NOT pulled in here —
// this umbrella is engine-side and must stay game-agnostic. App TUs that need
// role visuals include "ui/RoleVisuals.h" directly.

// ── Platform dialogs ──────────────────────────────────────────────────────
#include <string>

std::string SystemOpenFileDialog();
std::string SystemSaveFileDialog(const std::string& defaultName);
