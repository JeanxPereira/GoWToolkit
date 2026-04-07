#pragma once
#include "imgui.h"
#include <filesystem>
#include <string>

// Config persistente — salvo em gowtool.gtkc (formato binário GTKC)
// Layout de janelas do ImGui é armazenado como blob opaco no final do arquivo
struct AppConfig {
  // Janela principal
  int windowX = 100;
  int windowY = 100;
  int windowW = 1280;
  int windowH = 720;
  bool maximized = false;

  // Accent color (ImGuiCol_ButtonActive / backdrop)
  float accentR = 0.880f; // default Custom Top Red
  float accentG = 0.150f;
  float accentB = 0.150f;
  float accentA = 1.000f;

  // UI scale + font size in pixels (synced with SettingsWindow)
  float uiScale = 1.0f;
  float fontSize = 14.0f;  // actual font pixel size (was fontScale in v5)
  std::string fontPath = "";

  // Window decoration mode
#ifdef __APPLE__
  bool nativeDecorations = true; // macOS: native traffic lights by default
  bool nativeMenuBar = true;     // macOS: use NSMenu system menu bar
#else
  bool nativeDecorations = true; // Windows/Linux: custom borderless by default
  bool nativeMenuBar = true;
#endif

  // Audio
  float audioVolume = 1.0f;

  // Viewport Rendering Custom Colors
  float bgTopR = 0.14f, bgTopG = 0.14f, bgTopB = 0.16f;
  float bgBotR = 0.06f, bgBotG = 0.06f, bgBotB = 0.08f;
  float boneR = 0.5f, boneG = 0.9f, boneB = 0.2f; // Defaults to the green from SceneRenderer
  float wireR = 0.0f, wireG = 0.0f, wireB = 0.0f;
  float matcapR = 0.62f, matcapG = 0.58f, matcapB = 0.56f;
  float gridR = 0.35f, gridG = 0.35f, gridB = 0.35f, gridA = 0.5f;
  float hlR = 1.0f, hlG = 1.0f, hlB = 0.0f, hlA = 1.0f; // Highlight outline color

  // Estado persistente das janelas/docking do ImGui
  std::string imguiIniState;

  // Load/Save usando formato binário exclusivo 'GTKC'
  static AppConfig load(const std::string &path);
  void save(const std::string &path) const;

  // Global Config Access
  static AppConfig* Get();
  static void SetInstance(AppConfig* cfg);

  // Aplica accent color ao ImGuiStyle
  void applyAccent() const;
};
