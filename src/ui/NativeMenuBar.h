#pragma once

// Native menu bar abstraction (1:1 ImHex menu:: namespace pattern).
// On macOS, dispatches to NSMenu system menu bar.
// On Windows/Linux, falls through to ImGui.

namespace NativeMenuBar {
    // Enable/disable native menu bar (only effective on macOS)
    void enable(bool enabled);
    bool isEnabled();

    // Main menu bar
    bool beginMainMenuBar();
    void endMainMenuBar();

    // Menus
    bool beginMenu(const char* label, bool enabled = true);
    void endMenu();

    // Items
    bool menuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);
    bool menuItemToggle(const char* label, const char* shortcut, bool* p_selected, bool enabled = true);

    // Separator
    void separator();
}
