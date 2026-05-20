#pragma once
#include "imgui.h"
#include <string>
#include <vector>

// ── GOW Font Manager ───────────────────────────────────────────────────────
// Centralized font management inspired by ImHex's font_loader.cpp.
//
// Owns the font list, atlas build, icon font merge, and GPU upload.
// Replaces the scattered logic previously in SettingsWindow, TitleBar,
// WindowDecorator, and App.
//
// Usage:
//   GOW::Fonts::Init();                        // enumerate system fonts
//   GOW::Fonts::SetIconFont({...});             // configure icon font
//   GOW::Fonts::BuildAtlas(fontIndex, size);    // build ImGui font atlas
//   // ... after ImGui::Render() ...
//   if (GOW::Fonts::IsPendingRebuild())
//       GOW::Fonts::UploadAtlas();              // GPU upload

namespace GOW::Fonts {

// ── Font entry (system or bundled font) ───────────────────────────────────
struct FontEntry {
    std::string label;
    std::string path;          // empty = ImGui default (ProggyClean)
    bool        pixelPerfect = false;
};

// ── Icon font configuration ──────────────────────────────────────────────
struct IconFontConfig {
    std::string path;
    ImWchar     rangeStart = 0xE000;
    ImWchar     rangeEnd   = 0xFA19;
    ImVec2      glyphOffset = {0.0f, 3.0f};
};

// ── Init ──────────────────────────────────────────────────────────────────
void Init();                                // populate system font list
void SetIconFont(const IconFontConfig& cfg); // configure the icon font to merge

// ── Font list ─────────────────────────────────────────────────────────────
const std::vector<FontEntry>& GetFontList();
int FindFontIndex(const std::string& path); // returns -1 if not found

// ── Atlas Build ───────────────────────────────────────────────────────────
// Builds the complete ImGui font atlas: base font + icon font merge.
// Uses ScaleManager for DPI-aware sizing.
// Does NOT invalidate GPU resources — caller must call UploadAtlas() after.
void BuildAtlas(int fontIndex, float fontSizePx);

// ── GPU Upload ────────────────────────────────────────────────────────────
// Destroys and recreates backend device objects (OpenGL textures).
// MUST be called outside the ImGui frame (after ImGui::Render()).
void UploadAtlas();

// ── Monospace font (always-on secondary atlas font) ──────────────────────
// Returned by BuildAtlas() each time it loads SFMono. May be nullptr on
// platforms / builds where the bundled mono font is missing; callers should
// fall back to the current font in that case (e.g. by passing nullptr to
// ImGui::PushFont, which leaves the current font active).
ImFont* GetMonoFont();

// ── State ─────────────────────────────────────────────────────────────────
bool   IsPendingRebuild();
void   RequestRebuild();
void   ClearPendingRebuild();
float  GetCurrentFontSize();
int    GetCurrentFontIndex();
const std::string& GetCurrentFontPath();

} // namespace GOW::Fonts
