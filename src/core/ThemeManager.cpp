#include "ThemeManager.h"
#include "platform/SystemTheme.h"
#include "imgui.h"
#include <cmath>

namespace GOW::Theme {

// ── Static state ──────────────────────────────────────────────────────────

static ImVec4 s_currentAccent = {0.880f, 0.150f, 0.150f, 1.0f};
static ThemeMode s_currentMode = ThemeMode::Dark;
static ThemeMode s_currentEffective = ThemeMode::Dark;
static std::map<int, ImVec4> s_overrides;
static FlashState s_flashState;

// Resolve System → Dark / Light via the platform helper. Cached at every
// ApplyTheme call rather than per-frame to avoid spawning popen() on Linux.
static ThemeMode ResolveMode(ThemeMode m) {
    if (m != ThemeMode::System) return m;
    auto sys = GOW::Platform::DetectSystemAppearance();
    return (sys == GOW::Platform::SystemAppearance::Dark) ? ThemeMode::Dark
                                                          : ThemeMode::Light;
}

// ── Smooth transition state ───────────────────────────────────────────────
static bool s_transitioning = false;
static float s_transitionStart = 0.0f;
static constexpr float kTransitionDuration = 0.25f; // seconds
static ImVec4 s_oldColors[ImGuiCol_COUNT];
static ImVec4 s_newColors[ImGuiCol_COUNT];

// ── Helpers ───────────────────────────────────────────────────────────────

static ImVec4 Alpha(ImVec4 c, float a) {
  return ImVec4(c.x, c.y, c.z, a);
}

static ImVec4 Dim(ImVec4 c, float f) {
  return ImVec4(c.x * f, c.y * f, c.z * f, c.w);
}

static ImVec4 Lerp4(const ImVec4 &a, const ImVec4 &b, float t) {
  return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

// ── Internal: compute accent-derived palette into a color array ───────────

static void ComputeAccentPalette(ImVec4 *colors, const ImVec4 &accent, ThemeMode mode) {
  // Start from ImGui's Dark or Light base. The accent overrides below then
  // re-tint the interactive surfaces on top of whichever base was loaded.
  ImGuiStyle tmp;
  if (mode == ThemeMode::Light) ImGui::StyleColorsLight(&tmp);
  else                          ImGui::StyleColorsDark (&tmp);
  for (int i = 0; i < ImGuiCol_COUNT; i++)
    colors[i] = tmp.Colors[i];

  // ── Frame backgrounds (accent-tinted) ──
  colors[ImGuiCol_FrameBg] = Alpha(accent, 0.15f);
  colors[ImGuiCol_FrameBgHovered] = Alpha(accent, 0.40f);
  colors[ImGuiCol_FrameBgActive] = Alpha(accent, 0.67f);

  // ── Highlight pure (accent full) ──
  colors[ImGuiCol_CheckMark] = accent;
  colors[ImGuiCol_SliderGrabActive] = accent;
  colors[ImGuiCol_ButtonActive] = Dim(accent, 0.9f);
  colors[ImGuiCol_HeaderActive] = accent;
  colors[ImGuiCol_SeparatorActive] = accent;
  colors[ImGuiCol_ResizeGripActive] = accent;
  colors[ImGuiCol_NavCursor] = accent;
  colors[ImGuiCol_InputTextCursor] = accent;

  // ── Tab system (COMPLETE) ──
  colors[ImGuiCol_TabHovered] = Alpha(accent, 0.80f);
  colors[ImGuiCol_Tab] = Alpha(accent, 0.30f);
  colors[ImGuiCol_TabSelected] = Alpha(accent, 0.60f);
  colors[ImGuiCol_TabSelectedOverline] = accent;
  colors[ImGuiCol_TabDimmed] = Alpha(accent, 0.15f);
  colors[ImGuiCol_TabDimmedSelected] = Alpha(accent, 0.40f);
  colors[ImGuiCol_TabDimmedSelectedOverline] = Alpha(accent, 0.50f);

  // ── Hover / interactive backgrounds ──
  colors[ImGuiCol_SliderGrab] = Alpha(accent, 0.80f);
  colors[ImGuiCol_Button] = Alpha(accent, 0.40f);
  colors[ImGuiCol_ButtonHovered] = accent;
  colors[ImGuiCol_Header] = Alpha(accent, 0.31f);
  colors[ImGuiCol_HeaderHovered] = Alpha(accent, 0.80f);
  colors[ImGuiCol_SeparatorHovered] = Alpha(accent, 0.78f);
  colors[ImGuiCol_ResizeGrip] = Alpha(accent, 0.20f);
  colors[ImGuiCol_ResizeGripHovered] = Alpha(accent, 0.67f);
  colors[ImGuiCol_TextSelectedBg] = Alpha(accent, 0.35f);

  // ── Docking ──
  colors[ImGuiCol_DockingPreview] = Alpha(accent, 0.70f);

  // ── Checkbox (ImGui 1.92+) ──
  colors[ImGuiCol_CheckboxSelectedBg] =
      Lerp4(colors[ImGuiCol_FrameBg], colors[ImGuiCol_FrameBgHovered], 0.65f);

  // ── Misc interactive ──
  colors[ImGuiCol_Separator] = Alpha(accent, 0.40f);
  colors[ImGuiCol_TitleBgActive] = Alpha(accent, 0.30f);
  colors[ImGuiCol_ScrollbarGrabActive] = accent;
  colors[ImGuiCol_ScrollbarGrabHovered] = Alpha(accent, 0.70f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
  colors[ImGuiCol_DragDropTarget] = Alpha(accent, 0.90f);
}

// ── ApplyTheme ────────────────────────────────────────────────────────────

void ApplyTheme(const ImVec4 &accent, ThemeMode mode, bool animate) {
  ImGuiStyle &s = ImGui::GetStyle();
  s_currentAccent     = accent;
  s_currentMode       = mode;            // store user's intent
  s_currentEffective  = ResolveMode(mode); // System → Dark/Light right here

  // Compute target palette
  ImVec4 target[ImGuiCol_COUNT];
  ComputeAccentPalette(target, accent, s_currentEffective);

  // Apply per-color overrides on top
  for (const auto &[idx, color] : s_overrides) {
    if (idx >= 0 && idx < ImGuiCol_COUNT)
      target[idx] = color;
  }

  if (animate && ImGui::GetTime() > 0.0) {
    // Snapshot current colors for lerp source
    for (int i = 0; i < ImGuiCol_COUNT; i++) {
      s_oldColors[i] = s.Colors[i];
      s_newColors[i] = target[i];
    }
    s_transitioning = true;
    s_transitionStart = (float)ImGui::GetTime();
  } else {
    // Instant apply (live color picker drag, or first frame)
    for (int i = 0; i < ImGuiCol_COUNT; i++)
      s.Colors[i] = target[i];
    s_transitioning = false;
  }
}

void ApplyTheme(const ImVec4 &accent, bool animate) {
  // Legacy shim: keep the currently-active mode (live colour-picker drags
  // shouldn't flip Dark↔Light unexpectedly).
  ApplyTheme(accent, s_currentMode, animate);
}

ThemeMode GetMode()          { return s_currentMode; }
ThemeMode GetEffectiveMode() { return s_currentEffective; }

void UpdateTransition() {
  if (!s_transitioning)
    return;

  float elapsed = (float)ImGui::GetTime() - s_transitionStart;
  float t = elapsed / kTransitionDuration;

  ImGuiStyle &s = ImGui::GetStyle();

  if (t >= 1.0f) {
    // Transition complete — snap to final
    for (int i = 0; i < ImGuiCol_COUNT; i++)
      s.Colors[i] = s_newColors[i];
    s_transitioning = false;
  } else {
    // Smooth cubic ease-out: t' = 1 - (1-t)^3
    float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    for (int i = 0; i < ImGuiCol_COUNT; i++)
      s.Colors[i] = Lerp4(s_oldColors[i], s_newColors[i], ease);
  }
}

bool IsTransitioning() { return s_transitioning; }

ImVec4 GetAccent() { return s_currentAccent; }

ImVec4 GetContrastColor(const ImVec4& fg, const ImVec4& bgBehind) {
  // Resolve a translucent fg over an opaque bgBehind: a 31%-alpha accent over a
  // dark window bg ends up much darker on screen than the raw accent suggests,
  // so we need to compute luminance on the actual blended pixel.
  const float a = fg.w;
  const float r = (1.0f - a) * bgBehind.x + a * fg.x;
  const float g = (1.0f - a) * bgBehind.y + a * fg.y;
  const float b = (1.0f - a) * bgBehind.z + a * fg.z;
  const float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;
  if (luminance > 0.45f) {
    return ImVec4(0.05f, 0.05f, 0.05f, 1.0f); // Nearly black
  } else {
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Pure white
  }
}

ImVec4 GetContrastColor(const ImVec4& bg) {
  // Treat the background as already opaque (legacy callers). Forward to the
  // alpha-aware form with an opaque copy of the same color so the blend is a
  // no-op and the result matches the original formula.
  ImVec4 opaque(bg.x, bg.y, bg.z, 1.0f);
  return GetContrastColor(opaque, opaque);
}

ImVec4 TextForSurface(const ImVec4 &surf) {
  // Resolve the translucent surface over the current opaque window bg.
  ImVec4 bgBehind = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
  const float a = surf.w;
  const float r = (1.0f - a) * bgBehind.x + a * surf.x;
  const float g = (1.0f - a) * bgBehind.y + a * surf.y;
  const float b = (1.0f - a) * bgBehind.z + a * surf.z;
  const float bgLum = 0.2126f * r + 0.7152f * g + 0.0722f * b;

  ImVec4 curText = ImGui::GetStyleColorVec4(ImGuiCol_Text);
  const float textLum =
      0.2126f * curText.x + 0.7152f * curText.y + 0.0722f * curText.z;

  // Perceptual luminance-difference gate. Only flip when the rendered surface
  // would clash with the current text colour. Otherwise keep the theme's text
  // colour so the UI stays visually consistent.
  constexpr float kMinDiff = 0.40f;
  if (std::fabs(bgLum - textLum) >= kMinDiff)
    return curText;
  return (bgLum > 0.5f) ? ImVec4(0.05f, 0.05f, 0.05f, 1.0f)
                        : ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
}

// ── Semantic toolbar tokens ───────────────────────────────────────────────

ImVec4 ToolbarButton() { return {0.15f, 0.15f, 0.18f, 0.85f}; }
ImVec4 ToolbarButtonHover() { return {0.25f, 0.25f, 0.30f, 0.90f}; }
ImVec4 ToolbarButtonActive() { return {0.35f, 0.35f, 0.40f, 0.95f}; }

// ── Per-color override storage ────────────────────────────────────────────

void SetColorOverride(int imguiColIdx, const ImVec4 &color) {
  s_overrides[imguiColIdx] = color;
  if (imguiColIdx >= 0 && imguiColIdx < ImGuiCol_COUNT)
    ImGui::GetStyle().Colors[imguiColIdx] = color;
}

void ClearColorOverride(int imguiColIdx) { s_overrides.erase(imguiColIdx); }

void ClearAllOverrides() { s_overrides.clear(); }

bool HasOverride(int imguiColIdx) {
  return s_overrides.find(imguiColIdx) != s_overrides.end();
}

// ── Flash state ───────────────────────────────────────────────────────────

FlashState &GetFlashState() { return s_flashState; }

// ── Color groups for the editor UI ────────────────────────────────────────

const std::vector<ColorGroup> &GetColorGroups() {
  // clang-format off
  static const std::vector<ColorGroup> groups = {
    {"Window", {
      {"Window Background",   ImGuiCol_WindowBg},
      {"Child Background",    ImGuiCol_ChildBg},
      {"Popup Background",    ImGuiCol_PopupBg},
      {"Border",              ImGuiCol_Border},
      {"Border Shadow",       ImGuiCol_BorderShadow},
    }},
    {"Text", {
      {"Text",                ImGuiCol_Text},
      {"Text Disabled",       ImGuiCol_TextDisabled},
      {"Text Link",           ImGuiCol_TextLink},
      {"Text Selected Bg",    ImGuiCol_TextSelectedBg},
    }},
    {"Frame", {
      {"Frame Bg",            ImGuiCol_FrameBg},
      {"Frame Bg Hovered",    ImGuiCol_FrameBgHovered},
      {"Frame Bg Active",     ImGuiCol_FrameBgActive},
    }},
    {"Title Bar", {
      {"Title Bg",            ImGuiCol_TitleBg},
      {"Title Bg Active",     ImGuiCol_TitleBgActive},
      {"Title Bg Collapsed",  ImGuiCol_TitleBgCollapsed},
      {"Menu Bar Bg",         ImGuiCol_MenuBarBg},
    }},
    {"Scrollbar", {
      {"Scrollbar Bg",        ImGuiCol_ScrollbarBg},
      {"Scrollbar Grab",      ImGuiCol_ScrollbarGrab},
      {"Scrollbar Hovered",   ImGuiCol_ScrollbarGrabHovered},
      {"Scrollbar Active",    ImGuiCol_ScrollbarGrabActive},
    }},
    {"Buttons", {
      {"Button",              ImGuiCol_Button},
      {"Button Hovered",      ImGuiCol_ButtonHovered},
      {"Button Active",       ImGuiCol_ButtonActive},
    }},
    {"Checkbox & Sliders", {
      {"Check Mark",          ImGuiCol_CheckMark},
      {"Checkbox Selected Bg",ImGuiCol_CheckboxSelectedBg},
      {"Slider Grab",         ImGuiCol_SliderGrab},
      {"Slider Grab Active",  ImGuiCol_SliderGrabActive},
    }},
    {"Headers", {
      {"Header",              ImGuiCol_Header},
      {"Header Hovered",      ImGuiCol_HeaderHovered},
      {"Header Active",       ImGuiCol_HeaderActive},
    }},
    {"Separators", {
      {"Separator",           ImGuiCol_Separator},
      {"Separator Hovered",   ImGuiCol_SeparatorHovered},
      {"Separator Active",    ImGuiCol_SeparatorActive},
    }},
    {"Resize Grip", {
      {"Resize Grip",         ImGuiCol_ResizeGrip},
      {"Resize Hovered",      ImGuiCol_ResizeGripHovered},
      {"Resize Active",       ImGuiCol_ResizeGripActive},
    }},
    {"Tabs", {
      {"Tab",                 ImGuiCol_Tab},
      {"Tab Hovered",         ImGuiCol_TabHovered},
      {"Tab Selected",        ImGuiCol_TabSelected},
      {"Tab Overline",        ImGuiCol_TabSelectedOverline},
      {"Tab Dimmed",          ImGuiCol_TabDimmed},
      {"Tab Dimmed Selected", ImGuiCol_TabDimmedSelected},
      {"Tab Dimmed Overline", ImGuiCol_TabDimmedSelectedOverline},
    }},
    {"Tables", {
      {"Table Header Bg",     ImGuiCol_TableHeaderBg},
      {"Table Border Strong", ImGuiCol_TableBorderStrong},
      {"Table Border Light",  ImGuiCol_TableBorderLight},
      {"Table Row Bg",        ImGuiCol_TableRowBg},
      {"Table Row Bg Alt",    ImGuiCol_TableRowBgAlt},
    }},
    {"Docking", {
      {"Docking Preview",     ImGuiCol_DockingPreview},
      {"Docking Empty Bg",    ImGuiCol_DockingEmptyBg},
    }},
    {"Navigation", {
      {"Nav Cursor",           ImGuiCol_NavCursor},
      {"Nav Windowing HL",     ImGuiCol_NavWindowingHighlight},
      {"Nav Windowing Dim",    ImGuiCol_NavWindowingDimBg},
      {"Input Text Cursor",    ImGuiCol_InputTextCursor},
    }},
    {"Misc", {
      {"Modal Dim Bg",         ImGuiCol_ModalWindowDimBg},
      {"Drag Drop Target",     ImGuiCol_DragDropTarget},
      {"Plot Lines",           ImGuiCol_PlotLines},
      {"Plot Lines Hovered",   ImGuiCol_PlotLinesHovered},
      {"Plot Histogram",       ImGuiCol_PlotHistogram},
      {"Plot Histogram Hov",   ImGuiCol_PlotHistogramHovered},
    }},
  };
  // clang-format on
  return groups;
}

} // namespace GOW::Theme
