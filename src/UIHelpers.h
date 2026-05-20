#pragma once

// ── Umbrella header ───────────────────────────────────────────────────────
// Provides all UI helper functions through focused sub-headers.
// Existing #include "UIHelpers.h" call sites continue to work unchanged.

// Formatting utilities (HashHex, FormatBytes, FormatNum, MatchesFilter)
#include "ui/Formatting.h"
// Bring GOW:: formatting helpers into global scope for backward compatibility
using GOW::HashHex;
using GOW::FormatBytes;
using GOW::FormatNum;
using GOW::MatchesFilter;

// TypeId → visual mapping (TypeName, ColorForType, IconForType, SkinModeName)
#include "ui/TypeVisuals.h"

// Role → visual mapping (ColorForRole, IconForRole) — GOWR-specific
#include "ui/RoleVisuals.h"

// ── Platform dialogs ──────────────────────────────────────────────────────
#include <string>

std::string SystemOpenFileDialog();
std::string SystemSaveFileDialog(const std::string& defaultName);
