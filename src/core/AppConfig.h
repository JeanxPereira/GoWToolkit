#pragma once
#include "imgui.h"
#include <string>
#include <filesystem>

// Config persistente — salvo em gowtool_config.ini
// Separado do gowtool_layout.ini do ImGui (que guarda só layout de janelas)
struct AppConfig {
    // Janela principal
    int   windowX      = 100;
    int   windowY      = 100;
    int   windowW      = 1280;
    int   windowH      = 720;
    bool  maximized    = false;

    // Accent color (ImGuiCol_ButtonActive / backdrop)
    float accentR      = 0.259f;   // default ImGui azul
    float accentG      = 0.588f;
    float accentB      = 0.980f;
    float accentA      = 1.000f;

    // UI / font scale (synced with SettingsWindow)
    float uiScale      = 1.0f;
    float fontScale    = 1.0f;

    // Window decoration mode
#ifdef __APPLE__
    bool nativeDecorations = true;   // macOS: native traffic lights by default
    bool nativeMenuBar     = true;   // macOS: use NSMenu system menu bar
#else
    bool nativeDecorations = false;  // Windows/Linux: custom borderless by default
    bool nativeMenuBar     = false;
#endif

    // Estado persistente das janelas/docking do ImGui
    std::string imguiIniState;

    // Load/Save usando formato binário exclusivo 'GCFG'
    static AppConfig load(const std::string& path);
    void save(const std::string& path) const;

    // Aplica accent color ao ImGuiStyle
    void applyAccent() const;
};
