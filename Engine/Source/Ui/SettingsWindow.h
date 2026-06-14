#pragma once
#include "imgui.h"
#include "Core/AppConfig.h"
#include "Core/ThemeManager.h"
#include "Ui/IPanel.h"
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

namespace Onyx::App {

// 1:1 ImHex ViewSettings layout: two-column table with category list on the
// left and settings on the right, using SubWindow grouping for subcategories.
class SettingsWindow : public IPanel {
public:
    // Reference to shared config — set in App::init
    AppConfig* config = nullptr;

    void Init();
    void Draw() override;
    std::string_view getName() const override { return "Settings"; }

private:
    // Category index (left panel selection)
    int m_selectedCategory = 0;

    // Interface settings state
    float m_uiScale       = 1.0f;
    float m_fontSize      = 14.0f;
    int   m_fontSelected  = 0;
    bool  m_fontSizeChanged = false; // deferred rebuild (ImHex pattern)
    bool  m_justOpened    = true;

    // Drawing helpers — one per category
    void DrawInterfaceCategory();
    void DrawAppearanceCategory();
    void DrawThemeEditorCategory();
    void DrawViewportCategory();
    void DrawAssetFiltersCategory();

    // Flash highlight state (ImHex-style per-color hover visualization)
    std::optional<int>    m_flashColorIdx;
    std::optional<ImVec4> m_flashOrigColor;

    // Accent color transition animation state
    bool    m_transitioningAccent = false;
    ImVec4  m_accentStartColor;
    ImVec4  m_accentTargetColor;
    float   m_accentTransitionStart = 0.0f;
};

} // namespace Onyx::App
