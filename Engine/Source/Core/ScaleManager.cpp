#include "Core/ScaleManager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>

namespace Onyx::Scale {

// â”€â”€ Static state â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static float s_userScale   = 1.0f;
static float s_nativeScale = 1.0f;

// Cached base style sizes (captured before any scaling is applied).
// Used by ApplyStyleScale() to reset before re-scaling.
static ImGuiStyle s_baseStyle;
static bool       s_baseStyleCaptured = false;

// â”€â”€ Init â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Init(float userScale, float nativeDpiScale) {
    s_userScale   = std::clamp(userScale, 0.5f, 3.0f);
    s_nativeScale = std::max(nativeDpiScale, 1.0f);

    // Capture the current style as the "base" before any scaling.
    // This must be called AFTER the theme has been applied but BEFORE
    // ScaleAllSizes() so we have a clean reference.
    if (!s_baseStyleCaptured) {
        s_baseStyle = ImGui::GetStyle();
        s_baseStyleCaptured = true;
    }
}

// â”€â”€ Queries â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

float GetUserScale()   { return s_userScale; }
float GetNativeScale() { return s_nativeScale; }
float GetGlobalScale() { return s_userScale * s_nativeScale; }
float GetFontDpi()     { return s_nativeScale * 96.0f; }

// â”€â”€ Mutations â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SetUserScale(float scale) {
    s_userScale = std::clamp(scale, 0.5f, 3.0f);
}

void SetNativeScale(float scale) {
    s_nativeScale = std::max(scale, 1.0f);
}

// â”€â”€ Helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

float Scaled(float value) {
    return value * GetGlobalScale();
}

ImVec2 Scaled(const ImVec2& v) {
    float s = GetGlobalScale();
    return ImVec2(v.x * s, v.y * s);
}

ImVec2 Scaled(float x, float y) {
    float s = GetGlobalScale();
    return ImVec2(x * s, y * s);
}

// â”€â”€ ApplyStyleScale â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Resets to the clean base style sizes, then applies the absolute scale.
// This prevents cumulative drift from repeated ScaleAllSizes(ratio) calls.

void ApplyStyleScale(float userScale) {
    if (!s_baseStyleCaptured)
        return;

    ImGuiStyle& current = ImGui::GetStyle();

    // Restore size-related fields from base (preserving colors)
    // Copy all non-color sizing fields from the base
    ImVec4 savedColors[ImGuiCol_COUNT];
    for (int i = 0; i < ImGuiCol_COUNT; i++)
        savedColors[i] = current.Colors[i];

    // Copy the entire base style (sizes + padding + rounding)
    current = s_baseStyle;

    // Restore current colors (theme may have changed since init)
    for (int i = 0; i < ImGuiCol_COUNT; i++)
        current.Colors[i] = savedColors[i];

    // Now apply absolute scale from clean base
    if (userScale != 1.0f)
        current.ScaleAllSizes(userScale);

    // Clamp minimum sizes to prevent visual artifacts
    current.SeparatorSize   = ImMax(current.SeparatorSize, 1.0f);
    current.ChildBorderSize = ImMax(current.ChildBorderSize, 0.0f);
    current.PopupBorderSize = ImMax(current.PopupBorderSize, 0.0f);
    current.FrameBorderSize = ImMax(current.FrameBorderSize, 0.0f);
    current.WindowBorderSize = ImMax(current.WindowBorderSize, 0.0f);
    current.TabBorderSize   = ImMax(current.TabBorderSize, 0.0f);
}

} // namespace Onyx::Scale
