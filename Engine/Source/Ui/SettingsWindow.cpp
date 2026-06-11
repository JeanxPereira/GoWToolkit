#define IMGUI_DEFINE_MATH_OPERATORS
#include "Ui/SettingsWindow.h"
#include "Core/PathUtils.h"
#include "Core/FontManager.h"
#include "Core/ScaleManager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Rendering/ShaderManager.h"
#include "Core/AssetVisibility.h"
#include "Fonts/SFSymbols.h"
#include "Ui/Widgets.h"

#include <algorithm>
#include <cstring>

// â”€â”€ SubWindow helpers (1:1 ImHex ImGuiExt::BeginSubWindow / EndSubWindow) â”€â”€

static bool BeginSubWindow(const char *label, ImVec2 size = ImVec2(0, 0)) {
  bool result = false;
  bool hasMenuBar = label != nullptr && label[0] != '\0';

  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
  ImGui::PushID("SubWindow");
  if (ImGui::BeginChild(
          label, size, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY,
          hasMenuBar ? ImGuiWindowFlags_MenuBar : ImGuiWindowFlags_None)) {
    result = true;
    if (hasMenuBar && ImGui::BeginMenuBar()) {
      ImGui::TextUnformatted(label);
      ImGui::EndMenuBar();
    }
  }
  ImGui::PopStyleVar();
  return result;
}

static void EndSubWindow() {
  ImGui::EndChild();
  ImGui::PopID();
}

// â”€â”€ Init â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SettingsWindow::Init() {
  // Font list is now managed by Onyx::Fonts (populated in App::init)
  if (config) {
    m_uiScale = config->uiScale;
    m_fontSize = config->fontSize;
    m_fontSelected = Onyx::Fonts::FindFontIndex(config->fontPath);
    if (m_fontSelected < 0) m_fontSelected = 0;
  }
  // Build the atlas for the first frame with the loaded config
  Onyx::Fonts::BuildAtlas(m_fontSelected, m_fontSize);
  // Persist back to config in case font index resolved differently
  if (config) {
    config->fontSize = m_fontSize;
    config->fontPath = Onyx::Fonts::GetCurrentFontPath();
  }
}

// â”€â”€ Draw (1:1 ImHex ViewSettings::drawContent layout) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SettingsWindow::Draw() {
  if (!visible) {
    m_justOpened = true;
    // Se a janela for fechada durante uma transiÃ§Ã£o de acento, finaliza na cor de destino
    if (m_transitioningAccent) {
      m_transitioningAccent = false;
      if (config) {
        config->accentR = m_accentTargetColor.x;
        config->accentG = m_accentTargetColor.y;
        config->accentB = m_accentTargetColor.z;
        config->accentA = m_accentTargetColor.w;
        Onyx::Theme::ApplyTheme(config->getAccent(),
                               (Onyx::Theme::ThemeMode)config->themeMode,
                               /*animate=*/false);
      }
    }
    return;
  }

  // Processa a transiÃ§Ã£o de cor de acento frame a frame
  if (m_transitioningAccent && config) {
    float elapsed = (float)ImGui::GetTime() - m_accentTransitionStart;
    constexpr float duration = 0.25f; // DuraÃ§Ã£o sincronizada com ThemeManager
    float t = elapsed / duration;
    if (t >= 1.0f) {
      m_transitioningAccent = false;
      config->accentR = m_accentTargetColor.x;
      config->accentG = m_accentTargetColor.y;
      config->accentB = m_accentTargetColor.z;
      config->accentA = m_accentTargetColor.w;
      Onyx::Theme::ApplyTheme(config->getAccent(),
                             (Onyx::Theme::ThemeMode)config->themeMode,
                             /*animate=*/false);
    } else {
      // Curva cubic ease-out para transiÃ§Ã£o suave
      float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
      config->accentR = m_accentStartColor.x + (m_accentTargetColor.x - m_accentStartColor.x) * ease;
      config->accentG = m_accentStartColor.y + (m_accentTargetColor.y - m_accentStartColor.y) * ease;
      config->accentB = m_accentStartColor.z + (m_accentTargetColor.z - m_accentStartColor.z) * ease;
      config->accentA = m_accentStartColor.w + (m_accentTargetColor.w - m_accentStartColor.w) * ease;
      Onyx::Theme::ApplyTheme(config->getAccent(),
                             (Onyx::Theme::ThemeMode)config->themeMode,
                             /*animate=*/false);
    }
  }

  if (m_justOpened) {
    m_justOpened = false;
    ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  }

  ImGui::SetNextWindowSizeConstraints(ImVec2(500, 300), ImVec2(1400, 800));

  if (!ImGui::Begin("Settings", &visible,
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) {
    ImGui::End();
    return;
  }

  // â”€â”€ Two-column table: categories | settings (1:1 ImHex) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  static const char *categories[] = {"Interface", "Appearance", "Theme Editor", "Viewport", "Asset Filters"};
  constexpr int categoryCount = 5;

  if (ImGui::BeginTable("Settings", 2,
                        ImGuiTableFlags_BordersInner |
                            ImGuiTableFlags_Resizable)) {
    ImGui::TableSetupColumn("##category", ImGuiTableColumnFlags_WidthFixed,
                            120.0f);
    ImGui::TableSetupColumn("##settings", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    // â”€â”€ Left: category list â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (int i = 0; i < categoryCount; i++) {
      if (ImGui::Selectable(categories[i], m_selectedCategory == i,
                            ImGuiSelectableFlags_NoAutoClosePopups)) {
        m_selectedCategory = i;
      }
    }

    // â”€â”€ Right: settings for selected category â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImGui::TableNextColumn();

    if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
      switch (m_selectedCategory) {
      case 0:
        DrawInterfaceCategory();
        break;
      case 1:
        DrawAppearanceCategory();
        break;
      case 2:
        DrawThemeEditorCategory();
        break;
      case 3:
        DrawViewportCategory();
        break;
      case 4:
        DrawAssetFiltersCategory();
        break;
      }
    }
    ImGui::EndChild();

    ImGui::EndTable();
  }

  ImGui::End();
}

#include "FontDebuggerWindow.h"

// â”€â”€ Category: Interface â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SettingsWindow::DrawInterfaceCategory() {
  ImGuiIO &io = ImGui::GetIO();
  ImGuiStyle &style = ImGui::GetStyle();

  static bool showFontDebug = false;
  if (Onyx::UI::Widgets::Button("Open SF Symbols Debugger")) {
    showFontDebug = true;
  }
  if (showFontDebug) {
    Onyx::FontDebuggerWindow::Draw(&showFontDebug);
  }
  ImGui::Separator();

  // â”€â”€ Sub: Scaling (uses ScaleManager for drift-free scaling) â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (BeginSubWindow("Scaling")) {
    ImGui::PushItemWidth(
        std::min(ImGui::GetContentRegionAvail().x - 120.0f, 300.0f));

    float uiScale = m_uiScale;
    if (ImGui::SliderFloat("UI Scale", &uiScale, 0.5f, 3.0f, "%.2fx")) {
      if (uiScale != m_uiScale && uiScale > 0.1f) {
        Onyx::Scale::SetUserScale(uiScale);
        Onyx::Scale::ApplyStyleScale(uiScale);
        Onyx::Theme::ApplyTheme(config->getAccent()); // reapply colors over reset style
        m_uiScale = uiScale;
        if (config)
          config->uiScale = uiScale;
      }
    }

    ImGui::BeginDisabled();
    ImGui::Indent();
    ImGui::TextWrapped("Scales padding, borders and widget sizes.");
    ImGui::NewLine();
    ImGui::Unindent();
    ImGui::EndDisabled();

    ImGui::Spacing();
    auto preset = [&](const char *lbl, float t) {
      if (Onyx::UI::Widgets::Button(lbl, ImVec2(50, 0))) {
        Onyx::Scale::SetUserScale(t);
        Onyx::Scale::ApplyStyleScale(t);
        Onyx::Theme::ApplyTheme(config->getAccent());
        m_uiScale = t;
        if (config)
          config->uiScale = t;
      }
      ImGui::SameLine();
    };
    preset("1x", 1.0f);
    preset("1.25x", 1.25f);
    preset("1.5x", 1.5f);
    preset("2x", 2.0f);
    ImGui::NewLine();

    ImGui::PopItemWidth();
  }
  EndSubWindow();

  ImGui::NewLine();

  // â”€â”€ Sub: Font (1:1 ImHex style) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (BeginSubWindow("Font")) {
    ImGui::PushItemWidth(
        std::min(ImGui::GetContentRegionAvail().x - 120.0f, 300.0f));

    // Font family (uses centralized Onyx::Fonts) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    const auto& fonts = Onyx::Fonts::GetFontList();
    bool familyChanged = false;
    if (ImGui::BeginCombo("Family", fonts[m_fontSelected].label.c_str())) {
      for (int i = 0; i < (int)fonts.size(); i++) {
        bool sel = (i == m_fontSelected);
        if (ImGui::Selectable(fonts[i].label.c_str(), sel)) {
          if (i != m_fontSelected) {
            m_fontSelected = i;
            familyChanged = true;
          }
        }
        if (sel)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // Font size (+/- input, deferred rebuild) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // Disabled for default pixel-perfect font (ProggyClean)
    bool isPixelPerfect = (m_fontSelected < (int)fonts.size() && fonts[m_fontSelected].pixelPerfect);
    ImGui::BeginDisabled(isPixelPerfect);
    {
      if (ImGui::InputFloat("Size", &m_fontSize, 1.0f, 2.0f, "%.0f pt")) {
        m_fontSize = std::clamp(m_fontSize, 8.0f, 36.0f);
        m_fontSizeChanged = true;
      }

      // Rebuild when input is deactivated (Enter key or lost focus)
      if (m_fontSizeChanged && ImGui::IsItemDeactivatedAfterEdit()) {
        m_fontSizeChanged = false;
        Onyx::Fonts::BuildAtlas(m_fontSelected, m_fontSize);
        if (config) { config->fontSize = m_fontSize; config->fontPath = Onyx::Fonts::GetCurrentFontPath(); }
      }
    }
    ImGui::EndDisabled();

    if (isPixelPerfect) {
      ImGui::BeginDisabled();
      ImGui::Indent();
      ImGui::TextWrapped("Select a TrueType font to change size.");
      ImGui::NewLine();
      ImGui::Unindent();
      ImGui::EndDisabled();
    }

    // Auto-rebuild on family change â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (familyChanged) {
      Onyx::Fonts::BuildAtlas(m_fontSelected, m_fontSize);
      if (config) { config->fontSize = m_fontSize; config->fontPath = Onyx::Fonts::GetCurrentFontPath(); }
    }

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("Preview");
    ImGui::TextUnformatted("The quick brown fox jumps over the lazy dog");
    ImGui::TextUnformatted("ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789");
    ImGui::TextDisabled("abcdefghijklmnopqrstuvwxyz !@#$%%^&*()");
  }
  EndSubWindow();

  ImGui::NewLine();

  // â”€â”€ Sub: Window â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (BeginSubWindow("Window")) {
    if (config) {
      bool native = config->nativeDecorations;
      if (ImGui::Checkbox("Use native window decorations", &native)) {
        config->nativeDecorations = native;
      }

      ImGui::BeginDisabled();
      ImGui::Indent();
      ImGui::TextWrapped("Controls titlebar style (native OS or custom "
                         "borderless). Requires restart.");
      ImGui::NewLine();
      ImGui::Unindent();
      ImGui::EndDisabled();

#if defined(GOWTOOL_OS_MACOS)
      bool nativeMenu = config->nativeMenuBar;
      if (ImGui::Checkbox("Use native menu bar", &nativeMenu)) {
        config->nativeMenuBar = nativeMenu;
      }

      ImGui::BeginDisabled();
      ImGui::Indent();
      ImGui::TextWrapped(
          "Place menus in the macOS system menu bar instead of the window.");
      ImGui::NewLine();
      ImGui::Unindent();
      ImGui::EndDisabled();
#endif
    }
  }
  EndSubWindow();
}
// â”€â”€ Category: Appearance â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SettingsWindow::DrawAppearanceCategory() {
  // â”€â”€ Sub: Theme Mode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (BeginSubWindow("Theme Mode")) {
    if (!config) {
      ImGui::TextDisabled("Config not available.");
    } else {
      int mode = (int)config->themeMode;
      bool changed = false;
      if (ImGui::RadioButton("Dark",   &mode, 0)) changed = true;
      ImGui::SameLine();
      if (ImGui::RadioButton("Light",  &mode, 1)) changed = true;
      ImGui::SameLine();
      if (ImGui::RadioButton("System", &mode, 2)) changed = true;
      // Show the resolved value when System is active.
      if (config->themeMode == 2) {
        ImGui::SameLine();
        const bool isDark = (Onyx::Theme::GetEffectiveMode() == Onyx::Theme::ThemeMode::Dark);
        ImGui::TextDisabled("(%s)", isDark ? "Dark" : "Light");
      }
      if (changed) {
        config->themeMode = (uint8_t)mode;
        Onyx::Theme::ApplyTheme(config->getAccent(),
                               (Onyx::Theme::ThemeMode)config->themeMode,
                               /*animate=*/true);
      }
    }
  }
  EndSubWindow();

  ImGui::NewLine();

  // â”€â”€ Sub: Accent Color (2-column layout: picker + presets) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (BeginSubWindow("Accent Color")) {
    if (!config) {
      ImGui::TextDisabled("Config not available.");
    } else if (ImGui::BeginTable("##accentLayout", 2,
                                 ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("picker",  ImGuiTableColumnFlags_WidthFixed,  240.0f);
      ImGui::TableSetupColumn("presets", ImGuiTableColumnFlags_WidthStretch);

      // â”€â”€ Column 0: color wheel â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);

      ImVec4 accent(config->accentR, config->accentG, config->accentB,
                    config->accentA);
      ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoSidePreview |
                                  ImGuiColorEditFlags_PickerHueWheel |
                                  ImGuiColorEditFlags_DisplayHex;
      ImGui::SetNextItemWidth(220.0f);
      if (ImGui::ColorPicker4("##accent", &accent.x, flags)) {
        if (ImGui::IsItemActive()) {
          m_transitioningAccent = false; // Interrompe a animaÃ§Ã£o apenas em caso de arrasto manual do usuÃ¡rio
        }
        config->accentR = accent.x;
        config->accentG = accent.y;
        config->accentB = accent.z;
        config->accentA = accent.w;
        Onyx::Theme::ApplyTheme(config->getAccent(),
                               (Onyx::Theme::ThemeMode)config->themeMode,
                               /*animate=*/false);
      }

      // â”€â”€ Column 1: built-in + custom presets â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
      ImGui::TableSetColumnIndex(1);
      ImGui::TextDisabled("Presets");

      struct Preset { const char *name; float r, g, b; };
      constexpr Preset presets[] = {
          {"Blue", 0.259f, 0.588f, 0.980f},  {"Purple", 0.502f, 0.200f, 0.900f},
          {"Green", 0.200f, 0.780f, 0.349f}, {"Orange", 0.980f, 0.490f, 0.100f},
          {"Red", 0.900f, 0.180f, 0.180f},   {"Cyan", 0.100f, 0.750f, 0.900f},
          {"Pink", 0.900f, 0.300f, 0.600f},  {"White", 0.850f, 0.850f, 0.850f},
      };

      auto applyAccent = [&](float r, float g, float b) {
        m_transitioningAccent = true;
        m_accentStartColor = ImVec4(config->accentR, config->accentG, config->accentB, config->accentA);
        m_accentTargetColor = ImVec4(r, g, b, 1.0f);
        m_accentTransitionStart = (float)ImGui::GetTime();
      };

      // Built-in row(s).
      for (int i = 0; i < (int)(sizeof(presets) / sizeof(presets[0])); i++) {
        const auto &p = presets[i];
        ImVec4 c(p.r, p.g, p.b, 1.0f);
        ImGui::PushID(i);
        if (ImGui::ColorButton("##bipreset", c,
                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                ImVec2(22, 22))) {
          applyAccent(p.r, p.g, p.b);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", p.name);
        ImGui::PopID();
        if ((i + 1) % 6 != 0) ImGui::SameLine();
      }

      // Custom presets â€” same grid, smaller batch sizes.
      if (!config->customPresets.empty()) {
        ImGui::NewLine();
        ImGui::TextDisabled("Custom");
        for (int i = 0; i < (int)config->customPresets.size(); i++) {
          auto &cp = config->customPresets[i];
          ImVec4 c(cp.r, cp.g, cp.b, 1.0f);
          ImGui::PushID(1000 + i);
          if (ImGui::ColorButton("##cpreset", c,
                  ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                  ImVec2(22, 22))) {
            applyAccent(cp.r, cp.g, cp.b);
          }
          if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", cp.name);
          if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
              config->customPresets.erase(config->customPresets.begin() + i);
              ImGui::EndPopup();
              ImGui::PopID();
              break;
            }
            ImGui::EndPopup();
          }
          ImGui::PopID();
          if ((i + 1) % 6 != 0) ImGui::SameLine();
        }
      }

      ImGui::NewLine();
      // Save-current button â€” opens a modal for the new preset's name.
      if (ImGui::Button("+ Save currentâ€¦")) {
        ImGui::OpenPopup("##savePreset");
      }
      if (ImGui::BeginPopup("##savePreset")) {
        static char buf[32] = {};
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputTextWithHint("##name", "Preset name", buf, sizeof(buf));
        ImGui::SameLine();
        const bool ok = ImGui::Button("Save");
        if (ok && buf[0] != '\0') {
          AppConfig::CustomPreset cp{};
          std::strncpy(cp.name, buf, sizeof(cp.name) - 1);
          cp.r = config->accentR;
          cp.g = config->accentG;
          cp.b = config->accentB;
          config->customPresets.push_back(cp);
          buf[0] = '\0';
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      ImGui::EndTable();
    }
  }
  EndSubWindow();
}

// â”€â”€ Category: Viewport â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SettingsWindow::DrawViewportCategory() {
  if (!config) {
    ImGui::TextDisabled("Config not available.");
    return;
  }

  ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoSidePreview |
                              ImGuiColorEditFlags_PickerHueWheel |
                              ImGuiColorEditFlags_DisplayHex;

  if (BeginSubWindow("Background")) {
    ImGui::TextDisabled("Gradient Colors");
    ImGui::ColorEdit3("Top Color", &config->bgTopR, flags);
    ImGui::ColorEdit3("Bottom Color", &config->bgBotR, flags);
  }
  EndSubWindow();
  ImGui::NewLine();

  if (BeginSubWindow("Grid Overlay")) {
    ImGui::ColorEdit4("Grid Color", &config->gridR, flags);
  }
  EndSubWindow();
  ImGui::NewLine();

  if (BeginSubWindow("Debugging Overlays")) {
    ImGui::ColorEdit3("Bones Color", &config->boneR, flags);
    ImGui::ColorEdit3("Wireframe Color", &config->wireR, flags);
    ImGui::ColorEdit4("Outline Color", &config->hlR, flags);
  }
  EndSubWindow();
  ImGui::NewLine();

  if (BeginSubWindow("Shading Base")) {
    if (ImGui::ColorEdit3("Matcap Base Color", &config->matcapR, flags)) {
      Onyx::ShaderManager::Get().GenerateMatcapTexture();
    }
  }
  EndSubWindow();
}

// â”€â”€ Category: Asset Filters â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SettingsWindow::DrawAssetFiltersCategory() {
    auto& vis = Onyx::AssetVisibility::Get();

    if (BeginSubWindow("Filters", ImVec2(0, 0))) {
        // â”€â”€ Reset button â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        if (Onyx::UI::Widgets::Button(ICON_SF_ARROW_COUNTERCLOCKWISE " Reset All")) {
            vis.ResetAllOverrides();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(restores default visibility)");
        ImGui::Separator();

        // â”€â”€ Per-game-version sections â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        struct GameSection {
            Onyx::GameVersion version;
            const char*      label;
        };
        static const GameSection sections[] = {
            { Onyx::GameVersion::GOW2, "God of War II (PS2)" },
            { Onyx::GameVersion::GOWR, "God of War Ragnarok" },
        };

        int totalHidden  = 0;
        int totalVisible = 0;

        for (const auto& section : sections) {
            auto types = vis.GetFilterableTypes(section.version);
            if (types.empty()) continue;

            // Separate into hidden-by-default and visible-by-default groups
            std::vector<Onyx::AssetVisibility::TypeVisInfo> hiddenDefaults;
            std::vector<Onyx::AssetVisibility::TypeVisInfo> visibleDefaults;

            for (const auto& t : types) {
                if (t.defaultVis == Onyx::Visibility::Hidden) {
                    hiddenDefaults.push_back(t);
                } else {
                    visibleDefaults.push_back(t);
                }
            }

            if (ImGui::TreeNodeEx(section.label, ImGuiTreeNodeFlags_DefaultOpen)) {

                // â”€â”€ Hidden by Default â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
                if (!hiddenDefaults.empty()) {
                    ImGui::TextDisabled("Hidden by default:");
                    ImGui::Indent(8.0f);

                    for (auto& t : hiddenDefaults) {
                        ImGui::PushID(static_cast<int>(t.id.value) +
                                      static_cast<int>(section.version) * 1000);

                        bool checked = t.currentlyVisible;
                        if (ImGui::Checkbox(t.name, &checked)) {
                            vis.SetUserOverride(section.version, t.id, checked);
                        }

                        // Show override indicator
                        if (t.hasOverride) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "*");
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("User override \xe2\x80\x94 click Reset to restore default");
                            }
                        }

                        if (t.currentlyVisible) totalVisible++;
                        else totalHidden++;

                        ImGui::PopID();
                    }
                    ImGui::Unindent(8.0f);
                }

                // â”€â”€ Visible by Default â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
                if (!visibleDefaults.empty()) {
                    ImGui::Spacing();
                    ImGui::TextDisabled("Visible by default:");
                    ImGui::Indent(8.0f);

                    for (auto& t : visibleDefaults) {
                        ImGui::PushID(static_cast<int>(t.id.value) +
                                      static_cast<int>(section.version) * 1000 + 500);

                        bool checked = t.currentlyVisible;
                        if (ImGui::Checkbox(t.name, &checked)) {
                            vis.SetUserOverride(section.version, t.id, checked);
                        }

                        if (t.hasOverride) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "*");
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("User override \xe2\x80\x94 click Reset to restore default");
                            }
                        }

                        if (t.currentlyVisible) totalVisible++;
                        else totalHidden++;

                        ImGui::PopID();
                    }
                    ImGui::Unindent(8.0f);
                }

                ImGui::TreePop();
            }
        }

        // â”€â”€ Footer stats â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        ImGui::Separator();
        ImGui::TextDisabled("Hidden: %d types  |  Visible: %d types", totalHidden, totalVisible);
    }
    EndSubWindow();
}

// â”€â”€ Category: Theme Editor (ImHex-style per-color editor) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SettingsWindow::DrawThemeEditorCategory() {
  if (!config) {
    ImGui::TextDisabled("Config not available.");
    return;
  }

  ImGui::TextDisabled(
      "Edit individual ImGui colors. Hover over a color to see it flash "
      "in the interface.");
  ImGui::Separator();
  ImGui::Spacing();

  auto &style = ImGui::GetStyle();
  bool anyHovered = false;

  for (const auto &group : Onyx::Theme::GetColorGroups()) {
    if (ImGui::CollapsingHeader(group.groupName)) {
      for (const auto &entry : group.entries) {
        // Restore original color if this was the previously flashed item
        if (m_flashOrigColor.has_value() &&
            m_flashColorIdx.has_value() &&
            *m_flashColorIdx == entry.imguiColIdx) {
          style.Colors[entry.imguiColIdx] = *m_flashOrigColor;
        }

        ImVec4 color = style.Colors[entry.imguiColIdx];

        // Override indicator
        if (Onyx::Theme::HasOverride(entry.imguiColIdx)) {
          ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "*");
          if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Custom override â€” right-click color to reset");
          ImGui::SameLine();
        }

        // Color picker (no input fields, with alpha bar)
        ImGui::PushID(entry.imguiColIdx);
        if (ImGui::ColorEdit4(
                entry.displayName, &color.x,
                ImGuiColorEditFlags_NoInputs |
                    ImGuiColorEditFlags_AlphaBar |
                    ImGuiColorEditFlags_AlphaPreviewHalf)) {
          // User changed a color â†’ store as override
          Onyx::Theme::SetColorOverride(entry.imguiColIdx, color);
        }

        // Right-click to reset individual override
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) &&
            Onyx::Theme::HasOverride(entry.imguiColIdx)) {
          Onyx::Theme::ClearColorOverride(entry.imguiColIdx);
          Onyx::Theme::ApplyTheme(config->getAccent());
        }

        // â”€â”€ Flash highlight on hover (ImHex-style) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        if (ImGui::IsItemHovered()) {
          anyHovered = true;
          if (!m_flashColorIdx.has_value()) {
            m_flashColorIdx = entry.imguiColIdx;
            m_flashOrigColor = color;
          }
        }
        ImGui::PopID();
      }
    }

    // Apply sinusoidal flash animation for the currently hovered color
    if (m_flashOrigColor.has_value() && m_flashColorIdx.has_value()) {
      ImVec4 flashColor = *m_flashOrigColor;

      // Sinusoidal pulse: lerp between dimmed original and bright white
      const float t =
          std::min(1.0f, (1.0f + sinf((float)ImGui::GetTime() * 6.0f)) / 2.0f);
      flashColor.x = flashColor.x * 0.5f + (1.0f - flashColor.x * 0.5f) * t;
      flashColor.y = flashColor.y * 0.5f + (1.0f - flashColor.y * 0.5f) * t;
      flashColor.z = flashColor.z * 0.5f;
      flashColor.w = 1.0f;

      style.Colors[*m_flashColorIdx] = flashColor;

      if (!anyHovered) {
        // Mouse left the color picker â€” restore original
        style.Colors[*m_flashColorIdx] = *m_flashOrigColor;
        m_flashColorIdx.reset();
        m_flashOrigColor.reset();
      }
    }
  }

  // â”€â”€ Reset all button â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  ImGui::Spacing();
  ImGui::Separator();
  if (Onyx::UI::Widgets::Button(ICON_SF_ARROW_COUNTERCLOCKWISE " Reset All Colors")) {
    Onyx::Theme::ClearAllOverrides();
    Onyx::Theme::ApplyTheme(config->getAccent());
  }
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Discard all individual color overrides and revert to accent-derived palette");
}
