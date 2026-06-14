#include <Onyx/Services/ThemeManager.h>
#include <Onyx/Platform/SystemTheme.h>
#include <Onyx/Services/Logger.h>
#include "imgui.h"
#include <cmath>
#include <algorithm>

namespace Onyx::Theme {

// â”€â”€ Static state â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

static ImVec4 s_currentAccent = {0.880f, 0.150f, 0.150f, 1.0f};
static ThemeMode s_currentMode = ThemeMode::Dark;
static ThemeMode s_currentEffective = ThemeMode::Dark;
static std::map<int, ImVec4> s_overrides;
static FlashState s_flashState;

// Resolve System â†’ Dark / Light via the platform helper. Cached at every
// ApplyTheme call rather than per-frame to avoid spawning popen() on Linux.
static ThemeMode ResolveMode(ThemeMode m) {
    if (m != ThemeMode::System) return m;
    auto sys = Onyx::Platform::DetectSystemAppearance();
    return (sys == Onyx::Platform::SystemAppearance::Dark) ? ThemeMode::Dark
                                                          : ThemeMode::Light;
}

// â”€â”€ Smooth transition state â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static bool s_transitioning = false;
static float s_transitionStart = 0.0f;
static constexpr float kTransitionDuration = 0.25f; // seconds
static ImVec4 s_oldColors[ImGuiCol_COUNT];
static ImVec4 s_newColors[ImGuiCol_COUNT];

// â”€â”€ Helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

// â”€â”€ Phase 1 helpers: luminance-aware surface tinting â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

// Relative luminance (BT.709 coefficients) of an opaque RGB color.
static float Luminance(const ImVec4& c) {
    return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
}

// Luminance of a translucent foreground composited over an opaque background.
static float BlendedLuminance(const ImVec4& fg, const ImVec4& bgOpaque) {
    const float a = fg.w;
    const float r = (1.0f - a) * bgOpaque.x + a * fg.x;
    const float g = (1.0f - a) * bgOpaque.y + a * fg.y;
    const float b = (1.0f - a) * bgOpaque.z + a * fg.z;
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// Produce an alpha-blended accent surface over bgOpaque that maintains at
// least |targetDelta| luminance distance from the background. Starts with
// baseAlpha; if the resulting surface is too close to the bg luminance, it
// solves for the alpha that pushes the blended pixel to bgLum Â± targetDelta,
// picking the side with more headroom.
static ImVec4 TintSurface(const ImVec4& accent, const ImVec4& bgOpaque,
                           float targetDelta, float baseAlpha) {
    // 1. Try the base alpha first.
    ImVec4 candidate = Alpha(accent, baseAlpha);
    float blendedLum = BlendedLuminance(candidate, bgOpaque);
    float bgLum = Luminance(bgOpaque);

    if (std::fabs(blendedLum - bgLum) >= targetDelta)
        return candidate; // Already satisfies the delta.

    // 2. Solve for alpha.
    // Blended luminance = (1 - a) * bgLum + a * accentLum
    //                   = bgLum + a * (accentLum - bgLum)
    // We want: |bgLum + a * (accentLum - bgLum) - bgLum| >= targetDelta
    //        = |a * (accentLum - bgLum)| >= targetDelta
    // So: a >= targetDelta / |accentLum - bgLum|   (when accent != bg lum)
    float accentLum = Luminance(accent);
    float lumDiff = accentLum - bgLum;

    float solvedAlpha;
    if (std::fabs(lumDiff) < 0.001f) {
        // Accent and bg are nearly the same luminance. No amount of alpha
        // will help with this accent color alone. Use max alpha and rely on
        // the invariant enforcement pass (Phase 3) to correct if needed.
        solvedAlpha = 0.95f;
    } else {
        // Pick direction: lighter or darker than bg, whichever has more room.
        float headroomUp   = 1.0f - bgLum;
        float headroomDown = bgLum;

        float targetLum;
        if (lumDiff > 0) {
            // Accent is lighter than bg â†’ go lighter (if room) or darker.
            targetLum = (headroomUp >= targetDelta) ? (bgLum + targetDelta)
                                                    : (bgLum - targetDelta);
        } else {
            // Accent is darker than bg â†’ go darker (if room) or lighter.
            targetLum = (headroomDown >= targetDelta) ? (bgLum - targetDelta)
                                                      : (bgLum + targetDelta);
        }

        // a = (targetLum - bgLum) / (accentLum - bgLum)
        solvedAlpha = (targetLum - bgLum) / lumDiff;
    }

    // Clamp to [0.10, 0.95] so the surface never disappears nor becomes
    // an opaque slab.
    solvedAlpha = std::clamp(solvedAlpha, 0.10f, 0.95f);
    return Alpha(accent, solvedAlpha);
}

// â”€â”€ Phase 2 helper: force minimum luminance delta â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

// Returns a luminance-shifted copy of `color` so that
// |Lum(returned) - Lum(ref)| >= minDelta. If already satisfied, returns
// `color` unmodified. Otherwise mixes toward white or black.
static ImVec4 ForceMinLumDelta(const ImVec4& color, const ImVec4& ref,
                                float minDelta) {
    float colorLum = Luminance(color);
    float refLum = Luminance(ref);

    if (std::fabs(colorLum - refLum) >= minDelta)
        return color;

    // Pick direction: away from ref, toward whichever endpoint has more room.
    ImVec4 target;
    float headroomUp   = 1.0f - refLum;
    float headroomDown = refLum;

    if (headroomUp >= headroomDown)
        target = ImVec4(1.0f, 1.0f, 1.0f, color.w); // mix toward white
    else
        target = ImVec4(0.0f, 0.0f, 0.0f, color.w); // mix toward black

    // Binary-search for the smallest mix factor that satisfies the delta.
    float lo = 0.0f, hi = 1.0f;
    ImVec4 result = color;
    for (int i = 0; i < 16; ++i) {
        float mid = (lo + hi) * 0.5f;
        result = Lerp4(color, target, mid);
        float resultLum = Luminance(result);
        if (std::fabs(resultLum - refLum) >= minDelta)
            hi = mid;
        else
            lo = mid;
    }
    // Use hi to guarantee the delta is met.
    result = Lerp4(color, target, hi);
    result.w = color.w; // preserve original alpha
    return result;
}

// Nudge a pure-accent indicator so it's visible against WindowBg. Only
// adjusts if accent luminance is within Â±threshold of the background.
static ImVec4 NudgeAccentIndicator(const ImVec4& accent, const ImVec4& bgOpaque,
                                    float threshold = 0.05f) {
    float aLum = Luminance(accent);
    float bLum = Luminance(bgOpaque);
    if (std::fabs(aLum - bLum) > threshold)
        return accent;

    // Push toward the opposite pole.
    if (bLum > 0.5f)
        return Lerp4(accent, ImVec4(0.0f, 0.0f, 0.0f, accent.w), 0.40f);
    else
        return Lerp4(accent, ImVec4(1.0f, 1.0f, 1.0f, accent.w), 0.40f);
}

// â”€â”€ Internal: compute accent-derived palette into a color array â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

static void ComputeAccentPalette(ImVec4 *colors, const ImVec4 &accent, ThemeMode mode) {
  // Start from ImGui's Dark or Light base. The accent overrides below then
  // re-tint the interactive surfaces on top of whichever base was loaded.
  ImGuiStyle tmp;
  if (mode == ThemeMode::Light) ImGui::StyleColorsLight(&tmp);
  else                          ImGui::StyleColorsDark (&tmp);
  for (int i = 0; i < ImGuiCol_COUNT; i++)
    colors[i] = tmp.Colors[i];

  // Reference background for luminance calculations.
  const ImVec4& bg = colors[ImGuiCol_WindowBg];

  // â”€â”€ Phase 1: luminance-aware accent surface tinting â”€â”€
  // Delta targets by interaction state:
  //   Normal  = 0.06  â€” subtle background tint
  //   Hovered = 0.12  â€” clear hover feedback
  //   Active  = 0.20  â€” strong pressed/active feedback
  constexpr float kDeltaNormal  = 0.06f;
  constexpr float kDeltaHovered = 0.12f;
  constexpr float kDeltaActive  = 0.20f;

  // â”€â”€ Frame backgrounds (accent-tinted) â”€â”€
  colors[ImGuiCol_FrameBg]        = TintSurface(accent, bg, kDeltaNormal,  0.15f);
  colors[ImGuiCol_FrameBgHovered] = TintSurface(accent, bg, kDeltaHovered, 0.40f);
  colors[ImGuiCol_FrameBgActive]  = TintSurface(accent, bg, kDeltaActive,  0.67f);

  // â”€â”€ Tab system â”€â”€
  colors[ImGuiCol_Tab]                     = TintSurface(accent, bg, kDeltaNormal,  0.30f);
  colors[ImGuiCol_TabHovered]              = TintSurface(accent, bg, kDeltaHovered, 0.80f);
  colors[ImGuiCol_TabSelected]             = TintSurface(accent, bg, kDeltaActive,  0.60f);
  colors[ImGuiCol_TabDimmed]               = TintSurface(accent, bg, kDeltaNormal,  0.15f);
  colors[ImGuiCol_TabDimmedSelected]       = TintSurface(accent, bg, kDeltaHovered, 0.40f);

  // Pure-accent overlines â€” nudge for visibility only.
  colors[ImGuiCol_TabSelectedOverline]      = NudgeAccentIndicator(accent, bg);
  colors[ImGuiCol_TabDimmedSelectedOverline] = Alpha(NudgeAccentIndicator(accent, bg), 0.50f);

  // â”€â”€ Buttons â”€â”€
  colors[ImGuiCol_Button]        = TintSurface(accent, bg, kDeltaNormal,  0.40f);
  colors[ImGuiCol_ButtonHovered] = TintSurface(accent, bg, kDeltaHovered, 0.80f);
  colors[ImGuiCol_ButtonActive]  = TintSurface(Dim(accent, 0.9f), bg, kDeltaActive, 0.90f);

  // â”€â”€ Headers â”€â”€
  colors[ImGuiCol_Header]        = TintSurface(accent, bg, kDeltaNormal,  0.31f);
  colors[ImGuiCol_HeaderHovered] = TintSurface(accent, bg, kDeltaHovered, 0.80f);
  colors[ImGuiCol_HeaderActive]  = TintSurface(accent, bg, kDeltaActive,  0.90f);

  // â”€â”€ Separators â”€â”€
  colors[ImGuiCol_Separator]        = TintSurface(accent, bg, kDeltaNormal,  0.40f);
  colors[ImGuiCol_SeparatorHovered] = TintSurface(accent, bg, kDeltaHovered, 0.78f);
  colors[ImGuiCol_SeparatorActive]  = TintSurface(accent, bg, kDeltaActive,  0.90f);

  // â”€â”€ Resize grip â”€â”€
  colors[ImGuiCol_ResizeGrip]        = TintSurface(accent, bg, kDeltaNormal,  0.20f);
  colors[ImGuiCol_ResizeGripHovered] = TintSurface(accent, bg, kDeltaHovered, 0.67f);
  colors[ImGuiCol_ResizeGripActive]  = TintSurface(accent, bg, kDeltaActive,  0.90f);

  // â”€â”€ Sliders â”€â”€
  colors[ImGuiCol_SliderGrab]       = TintSurface(accent, bg, kDeltaHovered, 0.80f);
  colors[ImGuiCol_SliderGrabActive] = NudgeAccentIndicator(accent, bg);

  // â”€â”€ Scrollbar â”€â”€
  colors[ImGuiCol_ScrollbarGrab]        = TintSurface(accent, bg, kDeltaNormal,  0.40f);
  colors[ImGuiCol_ScrollbarGrabHovered] = TintSurface(accent, bg, kDeltaHovered, 0.70f);
  colors[ImGuiCol_ScrollbarGrabActive]  = TintSurface(accent, bg, kDeltaActive,  0.90f);

  // â”€â”€ Title bar â”€â”€
  colors[ImGuiCol_TitleBgActive] = TintSurface(accent, bg, kDeltaNormal, 0.30f);

  // â”€â”€ Selection / drag-drop â”€â”€
  colors[ImGuiCol_TextSelectedBg] = TintSurface(accent, bg, kDeltaNormal,  0.35f);
  colors[ImGuiCol_DragDropTarget] = TintSurface(accent, bg, kDeltaActive,  0.90f);

  // â”€â”€ Docking â”€â”€
  colors[ImGuiCol_DockingPreview] = TintSurface(accent, bg, kDeltaHovered, 0.70f);

  // â”€â”€ Pure-accent indicators (nudge for visibility) â”€â”€
  colors[ImGuiCol_CheckMark]       = NudgeAccentIndicator(accent, bg);
  colors[ImGuiCol_NavCursor]       = NudgeAccentIndicator(accent, bg);
  colors[ImGuiCol_InputTextCursor] = NudgeAccentIndicator(accent, bg);

  // â”€â”€ Checkbox (ImGui 1.92+) â”€â”€
  colors[ImGuiCol_CheckboxSelectedBg] =
      Lerp4(colors[ImGuiCol_FrameBg], colors[ImGuiCol_FrameBgHovered], 0.65f);

  // â”€â”€ Misc â”€â”€
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);

  // â”€â”€ Phase 2: Global text recompute â”€â”€
  // Derive ImGuiCol_Text from the window background so it's always readable.
  colors[ImGuiCol_Text] = GetContrastColor(colors[ImGuiCol_WindowBg]);
  colors[ImGuiCol_TextDisabled] = Lerp4(colors[ImGuiCol_Text],
                                        colors[ImGuiCol_WindowBg], 0.50f);
  colors[ImGuiCol_TextLink] = ForceMinLumDelta(accent, colors[ImGuiCol_Text],
                                               0.15f);

  // â”€â”€ Phase 3: Mode-side surface luminance clamp â”€â”€
  //
  // ImGui has no per-state text color slot â€” Buttons, Tabs and Headers all
  // render their label with the global ImGuiCol_Text we just computed in
  // Phase 2. To keep that label readable on **every** state of **every**
  // accent surface, we cap the blended surface luminance to stay firmly on
  // the same side as the WindowBg.
  //
  //   Dark base  (white text) â†’ accent surfaces capped at lum â‰¤ kSurfLumMaxDark
  //   Light base (black text) â†’ accent surfaces floored at lum â‰¥ kSurfLumMinLight
  //
  // The bounds correspond to roughly a 3:1 WCAG contrast ratio against the
  // chosen text color, which is the recommended floor for non-body UI text.
  // A pure-accent indicator (CheckMark, NavCursor, â€¦) is exempt â€” it does
  // not carry text on top.
  constexpr float kSurfLumMaxDark   = 0.30f;
  constexpr float kSurfLumMinLight  = 0.70f;

  const float textLum = Luminance(colors[ImGuiCol_Text]);
  const bool  isDark  = textLum > 0.5f;                  // text is white-ish
  const float surfMax = isDark ? kSurfLumMaxDark : 1.0f;
  const float surfMin = isDark ? 0.0f           : kSurfLumMinLight;

  // List of accent-tinted surface slots subject to the clamp. Pure-accent
  // indicators (CheckMark, NavCursor, InputTextCursor, *Overline,
  // SliderGrabActive) are intentionally absent â€” they have no text on top.
  static const int kAccentSurfaceSlots[] = {
      ImGuiCol_FrameBg,            ImGuiCol_FrameBgHovered,     ImGuiCol_FrameBgActive,
      ImGuiCol_Tab,                ImGuiCol_TabHovered,         ImGuiCol_TabSelected,
      ImGuiCol_TabDimmed,          ImGuiCol_TabDimmedSelected,
      ImGuiCol_Button,             ImGuiCol_ButtonHovered,      ImGuiCol_ButtonActive,
      ImGuiCol_Header,             ImGuiCol_HeaderHovered,      ImGuiCol_HeaderActive,
      ImGuiCol_Separator,          ImGuiCol_SeparatorHovered,   ImGuiCol_SeparatorActive,
      ImGuiCol_ResizeGrip,         ImGuiCol_ResizeGripHovered,  ImGuiCol_ResizeGripActive,
      ImGuiCol_SliderGrab,
      ImGuiCol_TitleBgActive,
      ImGuiCol_ScrollbarGrab,      ImGuiCol_ScrollbarGrabHovered, ImGuiCol_ScrollbarGrabActive,
      ImGuiCol_DockingPreview,
      ImGuiCol_TextSelectedBg,     ImGuiCol_DragDropTarget,
  };

  // Binary-search the alpha that lands the blended surface luminance within
  // [surfMin, surfMax]. Walks toward the bg if the surface is too bright (or
  // too dark, in light mode); the bg side is always inside the bound because
  // the base palette put the WindowBg there.
  auto ClampSurfaceAlpha = [&](ImVec4& surf) {
      const float curLum = BlendedLuminance(surf, bg);
      if (curLum >= surfMin && curLum <= surfMax)
          return; // already inside the bound

      const float origAlpha = surf.w;
      // The bg side of [0, origAlpha] is alpha=0 (pure bg). If the bg is
      // outside the bound the user picked a contradictory WindowBg override
      // and we can't help â€” leave the surface alone.
      const float bgLum = Luminance(bg);
      if (bgLum < surfMin || bgLum > surfMax)
          return;

      float lo = 0.0f, hi = origAlpha;
      float bestAlpha = 0.0f;
      for (int i = 0; i < 24; ++i) {
          const float mid = (lo + hi) * 0.5f;
          ImVec4 trial = Alpha(surf, mid);
          const float trialLum = BlendedLuminance(trial, bg);
          if (trialLum >= surfMin && trialLum <= surfMax) {
              bestAlpha = mid;
              lo = mid; // try larger alpha that still satisfies bound
          } else {
              hi = mid;
          }
      }
      surf = Alpha(surf, bestAlpha);
  };

  for (int slot : kAccentSurfaceSlots)
      ClampSurfaceAlpha(colors[slot]);
}

// â”€â”€ ApplyTheme â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void ApplyTheme(const ImVec4 &accent, ThemeMode mode, bool animate) {
  ImGuiStyle &s = ImGui::GetStyle();
  s_currentAccent     = accent;
  s_currentMode       = mode;            // store user's intent
  s_currentEffective  = ResolveMode(mode); // System â†’ Dark/Light right here

  // Compute target palette (includes Phases 1â€“3 inside ComputeAccentPalette)
  ImVec4 target[ImGuiCol_COUNT];
  ComputeAccentPalette(target, accent, s_currentEffective);

  // â”€â”€ Phase 4: Apply per-color overrides on top â”€â”€
  // Overrides are intentional â€” we never auto-correct them. But we log a
  // warning when an override lands a surface on the wrong side of the
  // Phase 3 mode-side bound, since that almost certainly means unreadable
  // text on top of that surface.
  for (const auto &[idx, color] : s_overrides) {
    if (idx >= 0 && idx < ImGuiCol_COUNT) {
      const ImVec4& bg = target[ImGuiCol_WindowBg];
      const float textLum = Luminance(target[ImGuiCol_Text]);
      const bool  isDark  = textLum > 0.5f;
      const float surfLum = BlendedLuminance(color, bg);
      const float surfMax = isDark ? 0.30f : 1.0f;
      const float surfMin = isDark ? 0.0f  : 0.70f;
      if (surfLum < surfMin || surfLum > surfMax) {
        LOG_WARN("[ThemeManager] Override for slot %d (surfLum=%.2f) is "
                 "outside the mode bound [%.2f, %.2f]. Text may be "
                 "unreadable on this surface.",
                 idx, surfLum, surfMin, surfMax);
      }
      target[idx] = color;
    }
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
  // shouldn't flip Darkâ†”Light unexpectedly).
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
    // Transition complete â€” snap to final
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

// â”€â”€ Phase 4: Semantic toolbar tokens â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Derived from the recomputed accent palette rather than hardcoded values.

ImVec4 ToolbarButton() {
    ImVec4 btn = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    // Subtle variant: reduce alpha slightly for toolbar's more subdued look.
    return Alpha(btn, std::max(btn.w * 0.85f, 0.10f));
}

ImVec4 ToolbarButtonHover() {
    return ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
}

ImVec4 ToolbarButtonActive() {
    return ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
}

ImVec4 ToolbarButtonText() {
    return TextForSurface(ToolbarButton());
}

// â”€â”€ Per-color override storage â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

// â”€â”€ Flash state â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

FlashState &GetFlashState() { return s_flashState; }

// â”€â”€ Color groups for the editor UI â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

} // namespace Onyx::Theme
