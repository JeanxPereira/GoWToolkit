#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui/SettingsWindow.h"
#include "ui/AppContext.h"
#include "ui/TitleBar.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "core/PathUtils.h"
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// ── SubWindow helpers (1:1 ImHex ImGuiExt::BeginSubWindow / EndSubWindow) ──

static bool BeginSubWindow(const char* label, ImVec2 size = ImVec2(0, 0)) {
    bool result = false;
    bool hasMenuBar = label != nullptr && label[0] != '\0';

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::PushID("SubWindow");
    if (ImGui::BeginChild(label, size,
            ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY,
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

// ── Init ────────────────────────────────────────────────────────────────────

void SettingsWindow::Init() {
    PopulateFontList();
    if (config) {
        m_uiScale = config->uiScale;
        m_fontGlobalScale = config->fontScale;
    }
}

void SettingsWindow::PopulateFontList() {
    m_fonts.clear();
    m_fonts.push_back({"Default (ProggyClean)", "", 13.0f});

#ifdef _WIN32
    const char* paths[]  = {
        "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/verdana.ttf", "C:/Windows/Fonts/tahoma.ttf",
        "C:/Windows/Fonts/JetBrainsMono-Regular.ttf"
    };
    const char* labels[] = {
        "Segoe UI", "Segoe UI Bold", "Consolas", "Arial", "Verdana", "Tahoma",
        "JetBrains Mono"
    };
    for (int i = 0; i < 7; i++)
        if (GetFileAttributesA(paths[i]) != INVALID_FILE_ATTRIBUTES)
            m_fonts.push_back({labels[i], paths[i], m_fontSize});
#elif defined(__APPLE__)
    struct { const char* label; const char* path; } macFonts[] = {
        {"SF Mono",    "/System/Library/Fonts/SFNSMono.ttf"},
        {"Menlo",      "/Library/Fonts/Menlo.ttc"},
        {"Monaco",     "/System/Library/Fonts/Monaco.ttf"},
        {"Courier New","/Library/Fonts/Courier New.ttf"},
        {"Helvetica",  "/System/Library/Fonts/Helvetica.ttc"},
        {"Arial",      "/Library/Fonts/Arial.ttf"},
    };
    for (const auto& f : macFonts)
        if (std::filesystem::exists(f.path))
            m_fonts.push_back({f.label, f.path, m_fontSize});
#endif
    // Bundled fonts (cross-platform, resolved relative to executable)
    struct { const char* label; const char* rel; } bundled[] = {
        {"SF Mono (bundled)", "third_party/fonts/SF-Mono-Regular.otf"},
        {"SF Mono (bundled)", "third_party/fonts/SFMono-Regular.otf"},
    };
    for (const auto& f : bundled) {
        auto abs = PathUtils::resolvePath(f.rel);
        if (std::filesystem::exists(abs))
            m_fonts.push_back({f.label, abs, m_fontSize});
    }
}

void SettingsWindow::RebuildFontAtlas() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    const FontEntry& fe = m_fonts[m_fontSelected];
    if (fe.path.empty())
        io.Fonts->AddFontDefault();
    else {
        ImFont* f = io.Fonts->AddFontFromFileTTF(fe.path.c_str(), m_fontSize);
        if (!f) io.Fonts->AddFontDefault();
    }

    TitleBar::loadIconFont(PathUtils::resolvePath("third_party/fonts/codicons.ttf").c_str(), m_fontSize + 1.0f);

    io.Fonts->Build();
    io.FontGlobalScale = m_fontGlobalScale;
    pendingFontRebuild = true;

    if (config) config->fontScale = m_fontGlobalScale;
}

void SettingsWindow::ApplyFontChange() {
    pendingFontRebuild = false;
}

void SettingsWindow::ApplyScaleClamp() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.SeparatorSize    = ImMax(s.SeparatorSize,    1.0f);
    s.ChildBorderSize  = ImMax(s.ChildBorderSize,  0.0f);
    s.PopupBorderSize  = ImMax(s.PopupBorderSize,  0.0f);
    s.FrameBorderSize  = ImMax(s.FrameBorderSize,  0.0f);
    s.WindowBorderSize = ImMax(s.WindowBorderSize, 0.0f);
    s.TabBorderSize    = ImMax(s.TabBorderSize,    0.0f);
}

// ── Draw (1:1 ImHex ViewSettings::drawContent layout) ───────────────────────

void SettingsWindow::draw(AppContext& /*ctx*/) {
    if (!visible) {
        m_justOpened = true;
        return;
    }

    if (m_justOpened) {
        m_justOpened = false;
        ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_Always);
        ImGui::SetNextWindowPos(
            ImGui::GetMainViewport()->GetCenter(),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 300), ImVec2(1400, 800));

    if (!ImGui::Begin("Settings", &visible,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) {
        ImGui::End();
        return;
    }

    // ── Two-column table: categories | settings (1:1 ImHex) ─────────────
    static const char* categories[] = { "Interface", "Appearance" };
    constexpr int categoryCount = 2;

    if (ImGui::BeginTable("Settings", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("##category", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("##settings", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // ── Left: category list ─────────────────────────────────────────
        for (int i = 0; i < categoryCount; i++) {
            if (ImGui::Selectable(categories[i], m_selectedCategory == i,
                    ImGuiSelectableFlags_NoAutoClosePopups)) {
                m_selectedCategory = i;
            }
        }

        // ── Right: settings for selected category ───────────────────────
        ImGui::TableNextColumn();

        if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
            switch (m_selectedCategory) {
                case 0: DrawInterfaceCategory(); break;
                case 1: DrawAppearanceCategory(); break;
            }
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }

    ImGui::End();
}

// ── Category: Interface ─────────────────────────────────────────────────────

void SettingsWindow::DrawInterfaceCategory() {
    ImGuiIO&    io    = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // ── Sub: Scaling ────────────────────────────────────────────────────
    if (BeginSubWindow("Scaling")) {
        ImGui::PushItemWidth(std::min(ImGui::GetContentRegionAvail().x - 120.0f, 300.0f));

        float uiScale = m_uiScale;
        if (ImGui::SliderFloat("UI Scale", &uiScale, 0.5f, 3.0f, "%.2fx")) {
            if (uiScale != m_uiScale && uiScale > 0.1f) {
                float ratio = uiScale / m_uiScale;
                style.ScaleAllSizes(ratio);
                ApplyScaleClamp();
                m_uiScale = uiScale;
                if (config) config->uiScale = uiScale;
            }
        }

        ImGui::BeginDisabled();
        ImGui::Indent();
        ImGui::TextWrapped("Scales padding, borders and widget sizes.");
        ImGui::NewLine();
        ImGui::Unindent();
        ImGui::EndDisabled();

        ImGui::Spacing();
        auto preset = [&](const char* lbl, float t) {
            if (ImGui::Button(lbl, ImVec2(50, 0))) {
                style.ScaleAllSizes(t / m_uiScale);
                ApplyScaleClamp();
                m_uiScale = t;
                if (config) config->uiScale = t;
            }
            ImGui::SameLine();
        };
        preset("1x", 1.0f); preset("1.25x", 1.25f);
        preset("1.5x", 1.5f); preset("2x", 2.0f);
        ImGui::NewLine();

        ImGui::Spacing();

        if (ImGui::SliderFloat("Font Scale", &m_fontGlobalScale, 0.5f, 3.0f, "%.2fx")) {
            io.FontGlobalScale = m_fontGlobalScale;
            if (config) config->fontScale = m_fontGlobalScale;
        }

        ImGui::BeginDisabled();
        ImGui::Indent();
        ImGui::TextWrapped("Scales font size without rebuilding the atlas.");
        ImGui::NewLine();
        ImGui::Unindent();
        ImGui::EndDisabled();

        ImGui::PopItemWidth();
    }
    EndSubWindow();

    ImGui::NewLine();

    // ── Sub: Font ───────────────────────────────────────────────────────
    if (BeginSubWindow("Font")) {
        ImGui::PushItemWidth(std::min(ImGui::GetContentRegionAvail().x - 120.0f, 300.0f));

        if (ImGui::BeginCombo("Family", m_fonts[m_fontSelected].label.c_str())) {
            for (int i = 0; i < (int)m_fonts.size(); i++) {
                bool sel = (i == m_fontSelected);
                if (ImGui::Selectable(m_fonts[i].label.c_str(), sel))
                    m_fontSelected = i;
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(70);
        if (ImGui::InputFloat("px", &m_fontSize, 1.0f, 2.0f, "%.0f"))
            m_fontSize = std::clamp(m_fontSize, 8.0f, 48.0f);

        ImGui::Spacing();
        if (ImGui::Button("Apply Font", ImVec2(130, 0)))
            RebuildFontAtlas();
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::TextUnformatted("(rebuilds atlas)");
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Preview");
        ImGui::TextUnformatted("The quick brown fox jumps over the lazy dog");
        ImGui::TextUnformatted("ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789");
        ImGui::TextDisabled("abcdefghijklmnopqrstuvwxyz !@#$%%^&*()");
    }
    EndSubWindow();

    ImGui::NewLine();

    // ── Sub: Window ─────────────────────────────────────────────────────
    if (BeginSubWindow("Window")) {
        if (config) {
            bool native = config->nativeDecorations;
            if (ImGui::Checkbox("Use native window decorations", &native)) {
                config->nativeDecorations = native;
            }

            ImGui::BeginDisabled();
            ImGui::Indent();
            ImGui::TextWrapped("Controls titlebar style (native OS or custom borderless). Requires restart.");
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
            ImGui::TextWrapped("Place menus in the macOS system menu bar instead of the window.");
            ImGui::NewLine();
            ImGui::Unindent();
            ImGui::EndDisabled();
#endif
        }
    }
    EndSubWindow();
}

// ── Category: Appearance ────────────────────────────────────────────────────

void SettingsWindow::DrawAppearanceCategory() {
    // ── Sub: Accent Color ───────────────────────────────────────────────
    if (BeginSubWindow("Accent Color")) {
        if (!config) {
            ImGui::TextDisabled("Config not available.");
        } else {
            ImVec4 accent(config->accentR, config->accentG, config->accentB, config->accentA);

            ImGuiColorEditFlags flags =
                ImGuiColorEditFlags_NoSidePreview |
                ImGuiColorEditFlags_PickerHueWheel |
                ImGuiColorEditFlags_DisplayRGB |
                ImGuiColorEditFlags_DisplayHex;

            if (ImGui::ColorPicker4("##accent", &accent.x, flags)) {
                config->accentR = accent.x;
                config->accentG = accent.y;
                config->accentB = accent.z;
                config->accentA = accent.w;
                config->applyAccent();
            }

            ImGui::BeginDisabled();
            ImGui::Indent();
            ImGui::TextWrapped("Color used for buttons, sliders, titlebar backdrop and highlights.");
            ImGui::NewLine();
            ImGui::Unindent();
            ImGui::EndDisabled();
        }
    }
    EndSubWindow();

    ImGui::NewLine();

    // ── Sub: Presets ────────────────────────────────────────────────────
    if (BeginSubWindow("Presets")) {
        if (config) {
            struct Preset { const char* name; float r, g, b; };
            constexpr Preset presets[] = {
                {"Blue",    0.259f, 0.588f, 0.980f},
                {"Purple",  0.502f, 0.200f, 0.900f},
                {"Green",   0.200f, 0.780f, 0.349f},
                {"Orange",  0.980f, 0.490f, 0.100f},
                {"Red",     0.900f, 0.180f, 0.180f},
                {"Cyan",    0.100f, 0.750f, 0.900f},
                {"Pink",    0.900f, 0.300f, 0.600f},
                {"White",   0.850f, 0.850f, 0.850f},
            };

            int col = 0;
            for (const auto& p : presets) {
                ImVec4 c(p.r, p.g, p.b, 1.0f);
                ImGui::PushID(p.name);
                ImGui::PushStyleColor(ImGuiCol_Button,        c);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(c.x*1.1f, c.y*1.1f, c.z*1.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(c.x*0.9f, c.y*0.9f, c.z*0.9f, 1.0f));
                if (ImGui::Button(p.name, ImVec2(90, 28))) {
                    config->accentR = p.r; config->accentG = p.g;
                    config->accentB = p.b; config->accentA = 1.0f;
                    config->applyAccent();
                }
                ImGui::PopStyleColor(3);
                ImGui::PopID();
                if (++col % 4 != 0) ImGui::SameLine();
            }
            ImGui::NewLine();
        }
    }
    EndSubWindow();
}
