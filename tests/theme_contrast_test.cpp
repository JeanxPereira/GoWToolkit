// ── Appearance contrast invariant tests (doctest) ────────────────────────
//
// Verifies that the resolved theme keeps text readable on every accent-tinted
// surface, for a battery of extreme accent colours, in both Dark and Light.
//
// ── What this test used to do, and why it was wrong ──────────────────────
//
// It called Onyx::Theme::ApplyTheme() and then read ImGui::GetStyle(). Under
// Onyx v1.1 that reads nothing: ApplyTheme only Mutate()s the Appearance
// module's desired state, and the ImGuiStyle is written later by
// Appearance::Commit(), which Window drives once per frame outside the ImGui
// frame. With no Window there is no Commit, so the test was measuring
// ImGui's own StyleColorsDark defaults and calling them "the theme" -- which
// is why every accent produced identical numbers, why Light and Dark were
// indistinguishable, and why the failing value was always exactly
// (0.26, 0.59, 0.98), ImGui's default blue. The repo's own notes recorded
// those numbers as "the Onyx theme engine now returns 0.38-0.55"; they were
// never the theme engine's output at all.
//
// Appearance::Resolve(state, env) is the derivation itself, documented as
// "Pure -- safe to call from tests with no context". That is what this test
// uses now, so it measures what the app will actually display.
//
// ── What it asserts ──────────────────────────────────────────────────────
//
// The old assertion was a proxy: "surface luminance <= 0.35 in Dark", with a
// comment explaining the real intent, "so white text stays readable". A proxy
// breaks whenever the palette moves even if readability is fine. This asserts
// the intent directly -- the WCAG 2.1 contrast ratio between the theme's own
// text colour and each surface, composited over the window background.
//
// The 4.5:1 threshold is WCAG AA for normal-size text: an external standard,
// not a number picked to fit. Measured headroom at the time of writing is
// 5.48:1 (Dark, "Default red", HeaderActive) and 9.23:1 (Light), so this
// leaves real room without being slack.

#include <doctest/doctest.h>
#include <Onyx/Services/Appearance.h>
#include <Onyx/Services/ThemeManager.h>
#include "imgui.h"
#include <cmath>
#include <utility>

// ── WCAG 2.1 contrast ────────────────────────────────────────────────────

static float SrgbToLinear(float c) {
    return (c <= 0.04045f) ? (c / 12.92f)
                           : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

static float RelLuminance(const ImVec4& c) {
    return 0.2126f * SrgbToLinear(c.x) +
           0.7152f * SrgbToLinear(c.y) +
           0.0722f * SrgbToLinear(c.z);
}

// Flattens a translucent colour onto an opaque background. A 31%-alpha accent
// over a dark window reaches the eye much darker than its raw value suggests,
// so contrast has to be computed on the pixel that actually gets drawn.
static ImVec4 Composite(const ImVec4& fg, const ImVec4& bg) {
    const float a = fg.w;
    return ImVec4((1.0f - a) * bg.x + a * fg.x,
                  (1.0f - a) * bg.y + a * fg.y,
                  (1.0f - a) * bg.z + a * fg.z,
                  1.0f);
}

static float ContrastRatio(const ImVec4& a, const ImVec4& b) {
    float la = RelLuminance(a), lb = RelLuminance(b);
    if (la < lb) std::swap(la, lb);
    return (la + 0.05f) / (lb + 0.05f);
}

// WCAG 2.1 AA, normal-size text.
static constexpr float kMinTextContrast = 4.5f;

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
struct SurfaceSlot { const char* name; int id; };

static const SurfaceSlot kSurfaceSlots[] = {
    {"FrameBg",           ImGuiCol_FrameBg},
    {"FrameBgHovered",    ImGuiCol_FrameBgHovered},
    {"FrameBgActive",     ImGuiCol_FrameBgActive},
    {"Tab",               ImGuiCol_Tab},
    {"TabHovered",        ImGuiCol_TabHovered},
    {"TabSelected",       ImGuiCol_TabSelected},
    {"TabDimmed",         ImGuiCol_TabDimmed},
    {"TabDimmedSelected", ImGuiCol_TabDimmedSelected},
    {"Button",            ImGuiCol_Button},
    {"ButtonHovered",     ImGuiCol_ButtonHovered},
    {"ButtonActive",      ImGuiCol_ButtonActive},
    {"Header",            ImGuiCol_Header},
    {"HeaderHovered",     ImGuiCol_HeaderHovered},
    {"HeaderActive",      ImGuiCol_HeaderActive},
};

static Onyx::Appearance::Resolved ResolveFor(const ImVec4& accent,
                                             Onyx::Theme::ThemeMode mode) {
    Onyx::Appearance::State st;
    st.accent = accent;
    st.mode   = mode;
    // Defaults: nativeScale 1, systemPrefersDark true. Neither matters here --
    // `mode` is always explicit, never System, so systemPrefersDark is unused.
    Onyx::Appearance::Environment env;
    return Onyx::Appearance::Resolve(st, env);
}

static void CheckReadable(Onyx::Theme::ThemeMode mode) {
    for (const auto& fix : kFixtures) {
        SUBCASE(fix.name) {
            const auto resolved = ResolveFor(fix.color, mode);
            const ImGuiStyle& s = resolved.style;

            const ImVec4 bg   = s.Colors[ImGuiCol_WindowBg];
            const ImVec4 text = s.Colors[ImGuiCol_Text];

            for (const auto& slot : kSurfaceSlots) {
                const ImVec4 surface = Composite(s.Colors[slot.id], bg);
                const float  ratio   = ContrastRatio(Composite(text, surface), surface);

                INFO("Accent: ", fix.name,
                     " | Slot: ", slot.name,
                     " | contrast=", ratio,
                     " | minRequired=", kMinTextContrast);

                CHECK(ratio >= kMinTextContrast);
            }
        }
    }
}

TEST_CASE("ThemeContrast: text stays readable on Dark-mode accent surfaces") {
    CheckReadable(Onyx::Theme::ThemeMode::Dark);
}

TEST_CASE("ThemeContrast: text stays readable on Light-mode accent surfaces") {
    CheckReadable(Onyx::Theme::ThemeMode::Light);
}

// The two modes must actually differ. Without this, a regression that made
// Resolve() ignore `mode` entirely -- which is exactly the failure the old
// version of this file could not see -- would leave both cases above passing
// against one palette.
TEST_CASE("ThemeContrast: Dark and Light resolve to different palettes") {
    for (const auto& fix : kFixtures) {
        SUBCASE(fix.name) {
            const ImVec4 darkBg  = ResolveFor(fix.color, Onyx::Theme::ThemeMode::Dark)
                                       .style.Colors[ImGuiCol_WindowBg];
            const ImVec4 lightBg = ResolveFor(fix.color, Onyx::Theme::ThemeMode::Light)
                                       .style.Colors[ImGuiCol_WindowBg];

            INFO("darkBgLum=", RelLuminance(darkBg),
                 " lightBgLum=", RelLuminance(lightBg));

            CHECK(RelLuminance(darkBg) < 0.2f);
            CHECK(RelLuminance(lightBg) > 0.6f);
        }
    }
}

// A theme that ignored the accent would also pass everything above, since the
// invariant is about readability, not hue. Two accents this far apart must
// produce visibly different surfaces.
TEST_CASE("ThemeContrast: the accent actually reaches the surfaces") {
    const auto red   = ResolveFor({0.88f, 0.15f, 0.15f, 1.0f}, Onyx::Theme::ThemeMode::Dark);
    const auto green = ResolveFor({0.2f, 1.0f, 0.4f, 1.0f},    Onyx::Theme::ThemeMode::Dark);

    int differing = 0;
    for (const auto& slot : kSurfaceSlots) {
        const ImVec4& a = red.style.Colors[slot.id];
        const ImVec4& b = green.style.Colors[slot.id];
        if (std::fabs(a.x - b.x) + std::fabs(a.y - b.y) + std::fabs(a.z - b.z) > 0.05f)
            ++differing;
    }
    INFO("slots differing between red and green accents: ", differing,
         " of ", (int)(sizeof(kSurfaceSlots) / sizeof(kSurfaceSlots[0])));
    CHECK(differing > 0);
}
