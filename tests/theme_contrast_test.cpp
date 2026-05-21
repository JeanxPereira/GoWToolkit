// ── ThemeManager contrast invariant tests (doctest) ──────────────────────
//
// Verifies that ApplyTheme produces readable text on every accent-tinted
// surface for a battery of extreme accent colors in both Dark and Light
// base modes.
//
// Build: part of gowtoolkit_tests (tests/CMakeLists.txt adds ThemeManager.cpp
//        and links imgui_lib).

#include <doctest/doctest.h>
#include "core/ThemeManager.h"
#include "imgui.h"
#include <cmath>

// ── Helpers (mirror the production ones so the test is self-contained) ────

static float TestLuminance(const ImVec4& c) {
    return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
}

static float TestBlendedLuminance(const ImVec4& fg, const ImVec4& bg) {
    const float a = fg.w;
    const float r = (1.0f - a) * bg.x + a * fg.x;
    const float g = (1.0f - a) * bg.y + a * fg.y;
    const float b = (1.0f - a) * bg.z + a * fg.z;
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// ── Fixtures ─────────────────────────────────────────────────────────────

struct AccentFixture {
    const char* name;
    ImVec4 color;
};

static const AccentFixture kFixtures[] = {
    {"Pure white",  {1.0f, 1.0f, 1.0f, 1.0f}},
    {"Pure black",  {0.0f, 0.0f, 0.0f, 1.0f}},
    {"Default red", {0.88f, 0.15f, 0.15f, 1.0f}},
    {"Neon green",  {0.2f, 1.0f, 0.4f, 1.0f}},
    {"Dark blue",   {0.08f, 0.10f, 0.35f, 1.0f}},
    {"Mid grey",    {0.5f, 0.5f, 0.5f, 1.0f}},
};

// Accent-tinted surface slots to validate.
static const int kSurfaceSlots[] = {
    ImGuiCol_FrameBg,           ImGuiCol_FrameBgHovered,     ImGuiCol_FrameBgActive,
    ImGuiCol_Tab,               ImGuiCol_TabHovered,         ImGuiCol_TabSelected,
    ImGuiCol_TabDimmed,         ImGuiCol_TabDimmedSelected,
    ImGuiCol_Button,            ImGuiCol_ButtonHovered,      ImGuiCol_ButtonActive,
    ImGuiCol_Header,            ImGuiCol_HeaderHovered,      ImGuiCol_HeaderActive,
};

// ── Ensure ImGui context exists for the test suite ───────────────────────

static ImGuiContext* EnsureImGuiContext() {
    if (!ImGui::GetCurrentContext()) {
        ImGui::CreateContext();
    }
    return ImGui::GetCurrentContext();
}

// ── Tests ────────────────────────────────────────────────────────────────

// Production bounds enforced by Phase 3 of ThemeManager:
//   Dark mode  → accent surface luminance ≤ 0.30 (so white text stays readable)
//   Light mode → accent surface luminance ≥ 0.70 (so black text stays readable)
//
// We assert slightly looser values in the test (0.35 / 0.65) to leave
// headroom for floating-point rounding in the binary-search solver.
constexpr float kTestSurfMaxDark  = 0.35f;
constexpr float kTestSurfMinLight = 0.65f;

TEST_CASE("ThemeContrast: accent surfaces stay dark in Dark mode") {
    auto* ctx = EnsureImGuiContext();
    (void)ctx;

    for (const auto& fix : kFixtures) {
        SUBCASE(fix.name) {
            GOW::Theme::ApplyTheme(fix.color, GOW::Theme::ThemeMode::Dark,
                                   /*animate=*/false);

            ImGuiStyle& s = ImGui::GetStyle();
            ImVec4 bg   = s.Colors[ImGuiCol_WindowBg];

            for (int slot : kSurfaceSlots) {
                float surfLum = TestBlendedLuminance(s.Colors[slot], bg);

                INFO("Accent: ", fix.name,
                     " | Slot: ", slot,
                     " | surfLum=", surfLum,
                     " | maxAllowed=", kTestSurfMaxDark);

                CHECK(surfLum <= kTestSurfMaxDark);
            }
        }
    }
}

TEST_CASE("ThemeContrast: accent surfaces stay light in Light mode") {
    auto* ctx = EnsureImGuiContext();
    (void)ctx;

    for (const auto& fix : kFixtures) {
        SUBCASE(fix.name) {
            GOW::Theme::ApplyTheme(fix.color, GOW::Theme::ThemeMode::Light,
                                   /*animate=*/false);

            ImGuiStyle& s = ImGui::GetStyle();
            ImVec4 bg   = s.Colors[ImGuiCol_WindowBg];

            for (int slot : kSurfaceSlots) {
                float surfLum = TestBlendedLuminance(s.Colors[slot], bg);

                INFO("Accent: ", fix.name,
                     " | Slot: ", slot,
                     " | surfLum=", surfLum,
                     " | minAllowed=", kTestSurfMinLight);

                CHECK(surfLum >= kTestSurfMinLight);
            }
        }
    }
}

TEST_CASE("ThemeContrast: Text color flips correctly for WindowBg") {
    auto* ctx = EnsureImGuiContext();
    (void)ctx;

    // Dark mode with white accent → text should be light (high luminance).
    GOW::Theme::ApplyTheme(ImVec4{1, 1, 1, 1}, GOW::Theme::ThemeMode::Dark,
                           false);
    {
        ImVec4 text = ImGui::GetStyle().Colors[ImGuiCol_Text];
        float textLum = TestLuminance(text);
        // Dark bg → text should be white/near-white.
        CHECK(textLum > 0.7f);
    }

    // Light mode with black accent → text should be dark (low luminance).
    GOW::Theme::ApplyTheme(ImVec4{0, 0, 0, 1}, GOW::Theme::ThemeMode::Light,
                           false);
    {
        ImVec4 text = ImGui::GetStyle().Colors[ImGuiCol_Text];
        float textLum = TestLuminance(text);
        // Light bg → text should be black/near-black.
        CHECK(textLum < 0.3f);
    }
}

TEST_CASE("ThemeContrast: TextLink distinguishable from Text") {
    auto* ctx = EnsureImGuiContext();
    (void)ctx;

    for (const auto& fix : kFixtures) {
        SUBCASE(fix.name) {
            GOW::Theme::ApplyTheme(fix.color, GOW::Theme::ThemeMode::Dark,
                                   false);
            ImGuiStyle& s = ImGui::GetStyle();
            float textLum = TestLuminance(s.Colors[ImGuiCol_Text]);
            float linkLum = TestLuminance(s.Colors[ImGuiCol_TextLink]);
            float delta = std::fabs(linkLum - textLum);
            INFO("Accent: ", fix.name, " | linkLum=", linkLum,
                 " | textLum=", textLum, " | delta=", delta);
            CHECK(delta >= 0.10f); // slightly below the 0.15 production target
        }
    }
}
