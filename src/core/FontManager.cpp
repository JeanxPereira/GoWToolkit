#include "core/FontManager.h"
#include "core/PathUtils.h"
#include "core/ScaleManager.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace GOW::Fonts {

// ── Static state ──────────────────────────────────────────────────────────
static std::vector<FontEntry> s_fonts;
static IconFontConfig         s_iconCfg;

static int         s_currentFontIndex = 0;
static float       s_currentFontSize  = 14.0f;
static bool        s_pendingRebuild   = false;
static ImFont*     s_monoFont         = nullptr;

// ── Init: populate system font list ───────────────────────────────────────

void Init() {
    s_fonts.clear();

    // Default ImGui font (ProggyClean — pixel-perfect bitmap)
    s_fonts.push_back({"Default (ProggyClean)", "", true});

#ifdef _WIN32
    const char* paths[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/verdana.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        "C:/Windows/Fonts/JetBrainsMono-Regular.ttf"
    };
    const char* labels[] = {
        "Segoe UI", "Segoe UI Bold", "Consolas", "Arial",
        "Verdana",  "Tahoma",        "JetBrains Mono"
    };
    for (int i = 0; i < 7; i++)
        if (GetFileAttributesA(paths[i]) != INVALID_FILE_ATTRIBUTES)
            s_fonts.push_back({labels[i], paths[i], false});

#elif defined(__APPLE__)
    struct { const char* label; const char* path; } macFonts[] = {
        {"SF Mono",      "/System/Library/Fonts/SFNSMono.ttf"},
        {"Menlo",        "/Library/Fonts/Menlo.ttc"},
        {"Monaco",       "/System/Library/Fonts/Monaco.ttf"},
        {"Courier New",  "/Library/Fonts/Courier New.ttf"},
        {"Helvetica",    "/System/Library/Fonts/Helvetica.ttc"},
        {"Arial",        "/Library/Fonts/Arial.ttf"},
    };
    for (const auto& f : macFonts)
        if (std::filesystem::exists(f.path))
            s_fonts.push_back({f.label, f.path, false});
#endif

    // Bundled fonts (cross-platform, resolved relative to executable)
    struct { const char* label; const char* rel; } bundled[] = {
        {"SF Mono (bundled)", "third_party/fonts/SF-Mono-Regular.otf"},
        {"SF Mono (bundled)", "third_party/fonts/SFMono-Regular.otf"},
    };
    for (const auto& f : bundled) {
        auto abs = PathUtils::resolvePath(f.rel);
        if (std::filesystem::exists(abs))
            s_fonts.push_back({f.label, abs, false});
    }
}

// ── SetIconFont ───────────────────────────────────────────────────────────

void SetIconFont(const IconFontConfig& cfg) {
    s_iconCfg = cfg;
}

// ── Font list queries ─────────────────────────────────────────────────────

const std::vector<FontEntry>& GetFontList() {
    return s_fonts;
}

int FindFontIndex(const std::string& path) {
    for (int i = 0; i < (int)s_fonts.size(); i++) {
        if (s_fonts[i].path == path)
            return i;
    }
    return -1;
}

// ── BuildAtlas ────────────────────────────────────────────────────────────
// Builds the complete ImGui font atlas with base font + icon font merge.
// Does NOT upload to GPU — caller must call UploadAtlas() after rendering.

void BuildAtlas(int fontIndex, float fontSizePx) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // Clamp inputs
    fontIndex = std::clamp(fontIndex, 0, (int)s_fonts.size() - 1);
    fontSizePx = std::clamp(fontSizePx, 8.0f, 72.0f);

    const FontEntry& fe = s_fonts[fontIndex];

    // ── Base font ─────────────────────────────────────────────────────
    // ImGui 1.92+: destination font must use explicit reference size so the
    // merged icon font (MergeMode) can also be explicit. Otherwise ImGui
    // asserts: "Cannot use MergeMode with an explicit reference size when
    // the destination font used an implicit reference size!"
    ImFontConfig baseCfg;
    baseCfg.SizePixels = fontSizePx;

    if (fe.path.empty() || fe.pixelPerfect) {
        io.Fonts->AddFontDefault(&baseCfg);
    } else {
        ImFont* f = io.Fonts->AddFontFromFileTTF(fe.path.c_str(), fontSizePx, &baseCfg);
        if (!f)
            io.Fonts->AddFontDefault(&baseCfg);
    }

    // ── Merge icon font (SFSymbols) ───────────────────────────────────
    if (!s_iconCfg.path.empty() && std::filesystem::exists(s_iconCfg.path)) {
        float iconSize = fontSizePx + 1.0f;

        // Build glyph range (static storage required by ImGui)
        static ImWchar s_iconRanges[3];
        s_iconRanges[0] = s_iconCfg.rangeStart;
        s_iconRanges[1] = s_iconCfg.rangeEnd;
        s_iconRanges[2] = 0;

        ImFontConfig iconCfg;
        iconCfg.MergeMode = true;
        iconCfg.PixelSnapH = true;
        iconCfg.GlyphMinAdvanceX = iconSize;
        iconCfg.GlyphMaxAdvanceX = iconSize;
        iconCfg.GlyphOffset = s_iconCfg.glyphOffset;

        io.Fonts->AddFontFromFileTTF(
            s_iconCfg.path.c_str(), iconSize, &iconCfg, s_iconRanges);
    }

    // ── Mono font (secondary, not merged) ─────────────────────────────
    // Always register the bundled SFMono as a separate atlas entry so the
    // text editor viewer can PushFont it regardless of which base font the
    // user picked. Reset to nullptr first so a missing file leaves us in a
    // well-defined "no mono available" state for GetMonoFont() callers.
    s_monoFont = nullptr;
    {
        auto monoPath = PathUtils::resolvePath("third_party/fonts/SFMono-Regular.otf");
        if (std::filesystem::exists(monoPath)) {
            ImFontConfig monoCfg;
            monoCfg.SizePixels = fontSizePx;
            s_monoFont = io.Fonts->AddFontFromFileTTF(monoPath.c_str(), fontSizePx, &monoCfg);
        }
    }

    // ── Finalize ──────────────────────────────────────────────────────
    io.Fonts->Build();
    io.FontGlobalScale = 1.0f; // no global scaling — we use real pixel size

    // Update state
    s_currentFontIndex = fontIndex;
    s_currentFontSize  = fontSizePx;
    s_pendingRebuild   = true;
}

// ── UploadAtlas ───────────────────────────────────────────────────────────
// Destroys and recreates OpenGL device objects (font texture).
// MUST be called outside ImGui frame (after ImGui::Render()).

void UploadAtlas() {
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGui_ImplOpenGL3_CreateDeviceObjects();
    s_pendingRebuild = false;
}

// ── State queries ─────────────────────────────────────────────────────────

bool    IsPendingRebuild()    { return s_pendingRebuild; }
void    RequestRebuild()      { s_pendingRebuild = true; }
void    ClearPendingRebuild() { s_pendingRebuild = false; }
float   GetCurrentFontSize()  { return s_currentFontSize; }
int     GetCurrentFontIndex() { return s_currentFontIndex; }
ImFont* GetMonoFont()         { return s_monoFont; }

const std::string& GetCurrentFontPath() {
    static const std::string empty;
    if (s_currentFontIndex >= 0 && s_currentFontIndex < (int)s_fonts.size())
        return s_fonts[s_currentFontIndex].path;
    return empty;
}

} // namespace GOW::Fonts
