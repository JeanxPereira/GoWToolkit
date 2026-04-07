#include "core/AppConfig.h"
#include "imgui.h"
#include <fstream>
#include <string>

#pragma pack(push, 1)
struct ConfigBinaryData {
    char magic[4]; // 'G','T','K','C' (v5+) or legacy 'G','C','F','G'
    uint32_t version;
    int windowX;
    int windowY;
    int windowW;
    int windowH;
    bool maximized;
    float accentR;
    float accentG;
    float accentB;
    float accentA;
    float uiScale;
    float fontScale;
    bool nativeDecorations;
    bool nativeMenuBar;
    float audioVolume;
    
    // V6 additions
    float bgTopR, bgTopG, bgTopB;
    float bgBotR, bgBotG, bgBotB;
    float boneR,  boneG,  boneB;
    float wireR,  wireG,  wireB;
    float matcapR, matcapG, matcapB;
    float gridR,  gridG,  gridB,  gridA;
    float hlR, hlG, hlB, hlA;

    uint32_t imguiStateSize;
    char fontPath[256];
};
#pragma pack(pop)

static AppConfig* s_instance = nullptr;

AppConfig* AppConfig::Get() {
    return s_instance;
}

void AppConfig::SetInstance(AppConfig* cfg) {
    s_instance = cfg;
}

// ── Load ───────────────────────────────────────────────────────────────────
AppConfig AppConfig::load(const std::string& path) {
    AppConfig cfg;
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return cfg;

    ConfigBinaryData data;
    if (f.read(reinterpret_cast<char*>(&data), sizeof(ConfigBinaryData))) {
        bool isGTKC = (data.magic[0] == 'G' && data.magic[1] == 'T' && data.magic[2] == 'K' && data.magic[3] == 'C');
        bool isGCFG = (data.magic[0] == 'G' && data.magic[1] == 'C' && data.magic[2] == 'F' && data.magic[3] == 'G');
        if (isGTKC || isGCFG) {
            cfg.windowX = data.windowX;
            cfg.windowY = data.windowY;
            cfg.windowW = data.windowW;
            cfg.windowH = data.windowH;
            cfg.maximized = data.maximized;
            cfg.accentR = data.accentR;
            cfg.accentG = data.accentG;
            cfg.accentB = data.accentB;
            cfg.accentA = data.accentA;
            cfg.uiScale = data.uiScale;
            cfg.fontSize = data.fontScale; // fontScale slot stores fontSize now

            // Migration: old configs stored FontGlobalScale (0.5–3.0)
            if (cfg.fontSize <= 5.0f) cfg.fontSize = 14.0f;

            if (data.version >= 2)
                cfg.nativeDecorations = data.nativeDecorations;
            if (data.version >= 3)
                cfg.nativeMenuBar = data.nativeMenuBar;
            if (data.version >= 4) {
                cfg.audioVolume = data.audioVolume;
            }
            if (data.version >= 6) {
                cfg.bgTopR = data.bgTopR; cfg.bgTopG = data.bgTopG; cfg.bgTopB = data.bgTopB;
                cfg.bgBotR = data.bgBotR; cfg.bgBotG = data.bgBotG; cfg.bgBotB = data.bgBotB;
                cfg.boneR  = data.boneR;  cfg.boneG  = data.boneG;  cfg.boneB  = data.boneB;
                cfg.wireR  = data.wireR;  cfg.wireG  = data.wireG;  cfg.wireB  = data.wireB;
                cfg.matcapR= data.matcapR;cfg.matcapG= data.matcapG;cfg.matcapB= data.matcapB;
                cfg.gridR  = data.gridR;  cfg.gridG  = data.gridG;  cfg.gridB  = data.gridB;  cfg.gridA = data.gridA;
                cfg.hlR    = data.hlR;    cfg.hlG    = data.hlG;    cfg.hlB    = data.hlB;    cfg.hlA   = data.hlA;
            }
            if (data.version >= 7) {
                char safeStr[257];
                std::strncpy(safeStr, data.fontPath, 256);
                safeStr[256] = '\0';
                cfg.fontPath = safeStr;
            }

            if (data.imguiStateSize > 0) {
                // Read embedded imgui config
                cfg.imguiIniState.resize(data.imguiStateSize);
                f.read(&cfg.imguiIniState[0], data.imguiStateSize);
            }
        }
    }
    return cfg;
}

// ── Save ───────────────────────────────────────────────────────────────────
void AppConfig::save(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return;

    ConfigBinaryData data;
    data.magic[0] = 'G'; data.magic[1] = 'T'; data.magic[2] = 'K'; data.magic[3] = 'C';
    data.version = 7;
    data.windowX = windowX;
    data.windowY = windowY;
    data.windowW = windowW;
    data.windowH = windowH;
    data.maximized = maximized;
    data.accentR = accentR;
    data.accentG = accentG;
    data.accentB = accentB;
    data.accentA = accentA;
    data.uiScale = uiScale;
    data.fontScale = fontSize; // fontSize stored in the fontScale binary slot
    data.nativeDecorations = nativeDecorations;
    data.nativeMenuBar = nativeMenuBar;
    data.audioVolume = audioVolume;
    
    data.bgTopR = bgTopR; data.bgTopG = bgTopG; data.bgTopB = bgTopB;
    data.bgBotR = bgBotR; data.bgBotG = bgBotG; data.bgBotB = bgBotB;
    data.boneR  = boneR;  data.boneG  = boneG;  data.boneB  = boneB;
    data.wireR  = wireR;  data.wireG  = wireG;  data.wireB  = wireB;
    data.matcapR = matcapR; data.matcapG = matcapG; data.matcapB = matcapB;
    data.gridR  = gridR;  data.gridG  = gridG;  data.gridB  = gridB;  data.gridA = gridA;
    data.hlR    = hlR;    data.hlG    = hlG;    data.hlB    = hlB;    data.hlA   = hlA;

    data.imguiStateSize = (uint32_t)imguiIniState.size();
    
    std::strncpy(data.fontPath, fontPath.c_str(), 255);
    data.fontPath[255] = '\0';

    f.write(reinterpret_cast<const char*>(&data), sizeof(ConfigBinaryData));
    
    if (data.imguiStateSize > 0) {
        f.write(imguiIniState.c_str(), data.imguiStateSize);
    }
}

// ── applyAccent ────────────────────────────────────────────────────────────
// Gera paleta de cores derivadas do accent para toda a UI
void AppConfig::applyAccent() const {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4 accent(accentR, accentG, accentB, accentA);

    // Cores derivadas do accent
    auto dim    = [](ImVec4 c, float f) { return ImVec4(c.x*f, c.y*f, c.z*f, c.w); };
    auto alpha  = [](ImVec4 c, float a) { return ImVec4(c.x, c.y, c.z, a); };

    s.Colors[ImGuiCol_ButtonActive]        = accent;
    s.Colors[ImGuiCol_ButtonHovered]       = dim(accent, 0.85f);
    s.Colors[ImGuiCol_Button]              = alpha(accent, 0.40f);

    s.Colors[ImGuiCol_CheckMark]           = accent;
    s.Colors[ImGuiCol_SliderGrab]          = dim(accent, 0.9f);
    s.Colors[ImGuiCol_SliderGrabActive]    = accent;

    s.Colors[ImGuiCol_HeaderHovered]       = alpha(accent, 0.45f);
    s.Colors[ImGuiCol_HeaderActive]        = alpha(accent, 0.65f);
    s.Colors[ImGuiCol_Header]              = alpha(accent, 0.30f);

    s.Colors[ImGuiCol_SeparatorHovered]    = alpha(accent, 0.70f);
    s.Colors[ImGuiCol_SeparatorActive]     = accent;

    s.Colors[ImGuiCol_ResizeGrip]          = alpha(accent, 0.30f);
    s.Colors[ImGuiCol_ResizeGripHovered]   = alpha(accent, 0.65f);
    s.Colors[ImGuiCol_ResizeGripActive]    = alpha(accent, 0.95f);

    s.Colors[ImGuiCol_FrameBg]             = alpha(accent, 0.20f);
    s.Colors[ImGuiCol_FrameBgHovered]      = alpha(accent, 0.40f);
    s.Colors[ImGuiCol_FrameBgActive]       = alpha(accent, 0.60f);
    
    s.Colors[ImGuiCol_TextSelectedBg]      = alpha(accent, 0.40f);

    s.Colors[ImGuiCol_TabSelected]         = dim(accent, 0.80f);
    s.Colors[ImGuiCol_TabHovered]          = alpha(accent, 0.70f);
    s.Colors[ImGuiCol_Tab]                 = alpha(accent, 0.30f);
    s.Colors[ImGuiCol_TabUnfocused]        = alpha(accent, 0.20f);
    s.Colors[ImGuiCol_TabUnfocusedActive]  = alpha(accent, 0.50f);

    s.Colors[ImGuiCol_Separator]           = alpha(accent, 0.40f);
    s.Colors[ImGuiCol_TitleBgActive]       = alpha(accent, 0.30f);

    s.Colors[ImGuiCol_NavHighlight]        = accent;

    // ScrollbarGrab é usado como hover dos botões da titlebar
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = accent;
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = alpha(accent, 0.70f);

    // Modal dim: dark transparent overlay (not white)
    s.Colors[ImGuiCol_ModalWindowDimBg]    = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
}
