#include "core/AppConfig.h"
#include "core/AssetVisibility.h"
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

    // V8 additions — camera projection settings
    float camFov;
    float camNearPlane;
    float camFarPlane;
    bool  camAutoNear;
    bool  camAutoFar;
    float camManualNear;
    float camManualFar;
    float camNearDistanceScale;
    float camFarMargin;
    float camNearFarRatioMax;
    bool  camPanelVisible;

    // V10 additions — theme mode (Dark/Light)
    uint8_t themeMode;
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

            if (data.version >= 8) {
                cfg.camFov               = data.camFov;
                cfg.camNearPlane         = data.camNearPlane;
                cfg.camFarPlane          = data.camFarPlane;
                cfg.camAutoNear          = data.camAutoNear;
                cfg.camAutoFar           = data.camAutoFar;
                cfg.camManualNear        = data.camManualNear;
                cfg.camManualFar         = data.camManualFar;
                cfg.camNearDistanceScale = data.camNearDistanceScale;
                cfg.camFarMargin         = data.camFarMargin;
                cfg.camNearFarRatioMax   = data.camNearFarRatioMax;
                cfg.camPanelVisible      = data.camPanelVisible;
            }

            if (data.imguiStateSize > 0) {
                // Read embedded imgui config
                cfg.imguiIniState.resize(data.imguiStateSize);
                f.read(&cfg.imguiIniState[0], data.imguiStateSize);
            }

            // V9: Asset visibility overrides (after ImGui state blob)
            if (data.version >= 9) {
                uint32_t overrideCount = 0;
                if (f.read(reinterpret_cast<char*>(&overrideCount), 4) && overrideCount > 0 && overrideCount < 1024) {
                    std::vector<GOW::AssetVisibility::SerializedOverride> overrides(overrideCount);
                    f.read(reinterpret_cast<char*>(overrides.data()),
                           overrideCount * sizeof(GOW::AssetVisibility::SerializedOverride));
                    GOW::AssetVisibility::Get().ImportOverrides(overrides);
                }
            }

            // V10: Theme mode (Dark/Light)
            if (data.version >= 10) {
                cfg.themeMode = data.themeMode;
            }

            // V11: Custom accent presets (tail blob after visibility overrides)
            if (data.version >= 11) {
                uint32_t presetCount = 0;
                if (f.read(reinterpret_cast<char*>(&presetCount), 4) &&
                    presetCount > 0 && presetCount < 256) {
                    cfg.customPresets.resize(presetCount);
                    f.read(reinterpret_cast<char*>(cfg.customPresets.data()),
                           presetCount * sizeof(AppConfig::CustomPreset));
                }
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
    data.version = 11;
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

    data.camFov               = camFov;
    data.camNearPlane         = camNearPlane;
    data.camFarPlane          = camFarPlane;
    data.camAutoNear          = camAutoNear;
    data.camAutoFar           = camAutoFar;
    data.camManualNear        = camManualNear;
    data.camManualFar         = camManualFar;
    data.camNearDistanceScale = camNearDistanceScale;
    data.camFarMargin         = camFarMargin;
    data.camNearFarRatioMax   = camNearFarRatioMax;
    data.camPanelVisible      = camPanelVisible;

    data.themeMode = themeMode;

    data.imguiStateSize = (uint32_t)imguiIniState.size();

    std::strncpy(data.fontPath, fontPath.c_str(), 255);
    data.fontPath[255] = '\0';

    f.write(reinterpret_cast<const char*>(&data), sizeof(ConfigBinaryData));
    
    if (data.imguiStateSize > 0) {
        f.write(imguiIniState.c_str(), data.imguiStateSize);
    }

    // V9: Asset visibility overrides
    auto overrides = GOW::AssetVisibility::Get().ExportOverrides();
    uint32_t overrideCount = (uint32_t)overrides.size();
    f.write(reinterpret_cast<const char*>(&overrideCount), 4);
    if (overrideCount > 0) {
        f.write(reinterpret_cast<const char*>(overrides.data()),
                overrideCount * sizeof(GOW::AssetVisibility::SerializedOverride));
    }

    // V11: Custom accent presets
    uint32_t presetCount = (uint32_t)customPresets.size();
    f.write(reinterpret_cast<const char*>(&presetCount), 4);
    if (presetCount > 0) {
        f.write(reinterpret_cast<const char*>(customPresets.data()),
                presetCount * sizeof(AppConfig::CustomPreset));
    }
}

