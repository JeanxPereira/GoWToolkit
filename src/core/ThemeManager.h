#pragma once
#include "imgui.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace GOW::Theme {

// ── Theme mode ────────────────────────────────────────────────────────────
// Picks the ImGui base palette before our accent overrides are applied.
// Dark = StyleColorsDark + accent; Light = StyleColorsLight + accent.
enum class ThemeMode : uint8_t {
    Dark   = 0,
    Light  = 1,
    System = 2,  // resolves to Dark or Light at ApplyTheme time via OS query
};

// ── Color entry for the per-color editor ──────────────────────────────────
struct ColorEntry {
  const char *displayName; // "Tab Hovered", "Button Active", etc.
  int imguiColIdx;         // ImGuiCol_TabHovered, etc.
};

struct ColorGroup {
  const char *groupName; // "Tabs", "Buttons", etc.
  std::vector<ColorEntry> entries;
};

// ── Main API ──────────────────────────────────────────────────────────────

// Apply full accent-derived theme to ImGui global style.
// animate=true for smooth transition (presets), false for instant (live drag).
//
// The (accent, animate) shim picks the currently-active ThemeMode so live
// drags of the colour picker do not unexpectedly flip Dark↔Light. The
// (accent, mode, animate) overload sets both at once and updates the cached
// mode for future shim calls.
void ApplyTheme(const ImVec4 &accent, bool animate = false);
void ApplyTheme(const ImVec4 &accent, ThemeMode mode, bool animate = false);

// Returns the user's stored ThemeMode intent (may be System).
ThemeMode GetMode();

// Returns the *effective* mode the palette was built against — always
// Dark or Light, never System. Use this for UI labels like "System (Dark)".
ThemeMode GetEffectiveMode();

// Must be called once per frame from main loop to drive smooth transitions.
void UpdateTransition();

// True while a smooth color transition is in progress.
bool IsTransitioning();

// Get current accent color (cached from last ApplyTheme call)
ImVec4 GetAccent();

// Returns white or black depending on the luminance of the provided background color.
ImVec4 GetContrastColor(const ImVec4& bg);

// Alpha-aware contrast: resolves a translucent fg over an opaque bgBehind first,
// then picks black/white based on the blended luminance. Use this when the
// foreground style color is alpha-blended (e.g. ImGuiCol_Header at 0.31 alpha
// over ImGuiCol_WindowBg) so the picked text color matches what the user
// actually sees on screen.
ImVec4 GetContrastColor(const ImVec4& fg, const ImVec4& bgBehind);

// Picks the best text colour for an interactive surface that will paint
// `surfaceColor` over the current ImGuiCol_WindowBg. If the luminance of the
// resolved (alpha-blended) surface is far enough from the current
// ImGuiCol_Text, the current text colour is returned unchanged. Otherwise the
// opposite endpoint (black or white) is returned so the label stays readable.
// Use for per-state contrast in widget wrappers (e.g. Button vs ButtonHovered
// with a bright accent).
ImVec4 TextForSurface(const ImVec4& surfaceColor);

// ── Semantic toolbar tokens (for localized PushStyleColor) ────────────────
ImVec4 ToolbarButton();
ImVec4 ToolbarButtonHover();
ImVec4 ToolbarButtonActive();

// ── Per-color editor support (ImHex-style) ────────────────────────────────

// Returns the list of color groups for the UI editor
const std::vector<ColorGroup> &GetColorGroups();

// Per-color override: stores an individual override for a specific ImGuiCol
void SetColorOverride(int imguiColIdx, const ImVec4 &color);
void ClearColorOverride(int imguiColIdx);
void ClearAllOverrides();
bool HasOverride(int imguiColIdx);

// ── Flash highlight state (for hover visualization) ───────────────────────
struct FlashState {
  std::optional<int> hoveredColorIdx;
  std::optional<ImVec4> originalColor;
};

FlashState &GetFlashState();

} // namespace GOW::Theme
