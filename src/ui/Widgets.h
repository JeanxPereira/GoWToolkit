#pragma once
#include "imgui.h"
#include <string>

namespace GOW::UI::Widgets {

    // Options for IconButton. All fields optional.
    //   size:     (0,0) ⇒ a square sized to ImGui::GetFrameHeight().
    //   tooltip:  shown after a short delay when hovered (nullptr ⇒ no tooltip).
    //   selected: paints the button as if it were pressed (toggle look). Uses
    //             the Header palette so it visually matches a selected tree row.
    //   disabled: greys out and disables clicks.
    struct IconButtonOpts {
        ImVec2      size      = ImVec2(0, 0);
        const char* tooltip   = nullptr;
        bool        selected  = false;
        bool        disabled  = false;
    };

    // A perfectly square button, sized to the current font height (plus padding).
    // Ideal for icons in toolbars or window title bars. The label text colour
    // auto-adapts to the active background luminance via Theme::TextForSurface.
    bool IconButton(const char* id, const char* icon, const IconButtonOpts& opts = {});

    // Legacy 3-arg shim — kept so existing callers keep building.
    bool IconButton(const char* id, const char* icon, const ImVec2& size);

    // A tree node that correctly handles text coloring on hover/selection
    // to avoid unreadable contrast (e.g. green text on blue background).
    // Automatically applies FramePadding for a taller, better-looking row.
    bool ColoredTreeNode(const char* id, const char* label, const char* icon, ImVec4 defaultColor, ImGuiTreeNodeFlags flags, bool isSelected = false);

    // ── Auto-contrast widget wrappers ──────────────────────────────────────
    // Each predicts hover state for the upcoming native widget rect, then
    // pushes the right ImGuiCol_Text via Theme::TextForSurface so the label
    // stays readable on any accent / theme mode. Drop-in replacements for the
    // matching ImGui:: functions.

    bool Button(const char* label, const ImVec2& size = ImVec2(0, 0));
    bool SmallButton(const char* label);
    bool Selectable(const char* label, bool selected = false,
                    ImGuiSelectableFlags flags = 0,
                    const ImVec2& size = ImVec2(0, 0));
    bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0);
    bool MenuItem(const char* label, const char* shortcut = nullptr,
                  bool selected = false, bool enabled = true);

    // BeginTabItem uses the previous frame's hover state cached per-ID since
    // tabs are laid out internally by the bar — one-frame latency on the very
    // first hover edge, indistinguishable at 60 fps. Pair with the native
    // ImGui::EndTabItem (no wrapper).
    bool BeginTabItem(const char* label, bool* p_open = nullptr,
                      ImGuiTabItemFlags flags = 0);

} // namespace GOW::UI::Widgets
