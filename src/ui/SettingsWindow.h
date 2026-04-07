#pragma once
#include "imgui.h"
#include "core/AppConfig.h"
#include "ui/IPanel.h"
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

struct FontEntry {
    std::string label;
    std::string path;
    float       size;
    ImFont*     handle = nullptr;
};

// 1:1 ImHex ViewSettings layout: two-column table with category list on the
// left and settings on the right, using SubWindow grouping for subcategories.
class SettingsWindow : public IPanel {
public:
    bool pendingFontRebuild = false;

    // Reference to shared config — set in App::init
    AppConfig* config = nullptr;

    void Init();
    void ApplyFontChange();
    void draw(AppContext& ctx) override;
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

    std::vector<FontEntry> m_fonts;

    // Drawing helpers — one per category
    void DrawInterfaceCategory();
    void DrawAppearanceCategory();
    void DrawViewportCategory();

    // Internal
    void PopulateFontList();
    void RebuildFontAtlas();
    void ApplyScaleClamp();
};
