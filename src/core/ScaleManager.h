#pragma once
#include "imgui.h"

// ── GOW Scale Manager ──────────────────────────────────────────────────────
// Centralized UI scaling system inspired by ImHex's System::getGlobalScale().
//
// Three scale factors:
//   - UserScale:   configured by the user in Settings (0.5–3.0)
//   - NativeScale: OS backing scale factor (macOS Retina = 2.0, others = 1.0)
//   - GlobalScale: UserScale × NativeScale (used by scaled() helpers)
//
// ApplyStyleScale() always resets to a clean ImGuiStyle base before scaling,
// preventing the cumulative drift bug from incremental ScaleAllSizes() calls.

namespace GOW::Scale {

// ── Init (called once by Window during startup) ───────────────────────────
void Init(float userScale, float nativeDpiScale);

// ── Queries ───────────────────────────────────────────────────────────────
float GetUserScale();       // user-configured factor (0.5–3.0)
float GetNativeScale();     // OS backing scale factor (macOS Retina=2.0)
float GetGlobalScale();     // user × native
float GetFontDpi();         // native × 96

// ── Mutations ─────────────────────────────────────────────────────────────
void SetUserScale(float scale);     // update + recompute global
void SetNativeScale(float scale);   // called on DPI change callback

// ── Helpers (1:1 ImHex scaled()) ──────────────────────────────────────────
float  Scaled(float value);
ImVec2 Scaled(const ImVec2& v);
ImVec2 Scaled(float x, float y);

// ── Style ─────────────────────────────────────────────────────────────────
// Applies ScaleAllSizes with the given absolute user scale.
// Resets to default sizes first, then scales — avoids cumulative drift.
void ApplyStyleScale(float userScale);

} // namespace GOW::Scale
