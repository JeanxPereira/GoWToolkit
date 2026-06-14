#include "Ui/NativeMenuBar.h"
#include "Ui/NativeWindow.h"
#include "imgui.h"

#if defined(GOWTOOL_OS_MACOS)
extern "C" {
    void macosmenu_init(void);
    void macosmenu_clear(void);

    bool macosmenu_beginMainMenuBar(void);
    void macosmenu_endMainMenuBar(void);

    bool macosmenu_beginMenu(const char* label, bool enabled);
    void macosmenu_endMenu(void);

    bool macosmenu_menuItem(const char* label, const char* shortcut,
                            bool selected, bool enabled);
    bool macosmenu_menuItemToggle(const char* label, const char* shortcut,
                                  bool* p_selected, bool enabled);

    void macosmenu_separator(void);
}
#endif

namespace Onyx::App::NativeMenuBar {

static bool s_enabled = false;
#if defined(GOWTOOL_OS_MACOS)
static bool s_initialized = false;
#endif

void enable(bool enabled) {
#if defined(GOWTOOL_OS_MACOS)
    // When switching from native to ImGui, clear the NSMenu items
    if (!enabled && s_enabled && s_initialized)
        macosmenu_clear();
    s_enabled = enabled;
#else
    (void)enabled;
#endif
}

bool isEnabled() {
    return s_enabled;
}

bool beginMainMenuBar() {
#if defined(GOWTOOL_OS_MACOS)
    if (s_enabled) {
        if (!s_initialized) {
            macosmenu_init();
            s_initialized = true;
        }
        return macosmenu_beginMainMenuBar();
    }
#endif
    return ImGui::BeginMainMenuBar();
}

void endMainMenuBar() {
#if defined(GOWTOOL_OS_MACOS)
    if (s_enabled) {
        macosmenu_endMainMenuBar();
        return;
    }
#endif
    ImGui::EndMainMenuBar();
}

bool beginMenu(const char* label, bool enabled) {
#if defined(GOWTOOL_OS_MACOS)
    if (s_enabled)
        return macosmenu_beginMenu(label, enabled);
#endif
    return ImGui::BeginMenu(label, enabled);
}

void endMenu() {
#if defined(GOWTOOL_OS_MACOS)
    if (s_enabled) {
        macosmenu_endMenu();
        return;
    }
#endif
    ImGui::EndMenu();
}

bool menuItem(const char* label, const char* shortcut, bool selected, bool enabled) {
#if defined(GOWTOOL_OS_MACOS)
    if (s_enabled)
        return macosmenu_menuItem(label, shortcut, selected, enabled);
#endif
    return ImGui::MenuItem(label, shortcut, selected, enabled);
}

bool menuItemToggle(const char* label, const char* shortcut, bool* p_selected, bool enabled) {
#if defined(GOWTOOL_OS_MACOS)
    if (s_enabled)
        return macosmenu_menuItemToggle(label, shortcut, p_selected, enabled);
#endif
    return ImGui::MenuItem(label, shortcut, p_selected, enabled);
}

void separator() {
#if defined(GOWTOOL_OS_MACOS)
    if (s_enabled) {
        macosmenu_separator();
        return;
    }
#endif
    ImGui::Separator();
}

} // namespace Onyx::App::NativeMenuBar
