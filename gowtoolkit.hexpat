// ══════════════════════════════════════════════════════════════════════════════
// GOWToolkit Configuration File Pattern (.gtkc)
// ImHex Pattern Language — https://docs.werwolv.net/pattern-language
// ══════════════════════════════════════════════════════════════════════════════
// File format: Pure binary header (ConfigBinaryData) followed by an opaque
// ImGui INI text blob. The header contains all GOWToolkit-specific settings;
// the blob is owned by Dear ImGui and treated as a black box.
//
// Magic: "GTKC" (v5+) or legacy "GCFG" (v1-v4)
// ══════════════════════════════════════════════════════════════════════════════

#pragma endian little

// ── GOWToolkit App Config Header ─────────────────────────────────────────────
struct ConfigBinaryData {
    char   magic[4];          // "GTKC" or legacy "GCFG"
    u32    version;           // Format version (current: 5)
    
    // Window geometry
    s32    windowX;
    s32    windowY;
    s32    windowW;
    s32    windowH;
    bool   maximized;
    
    // Accent color (RGBA float)
    float  accentR;
    float  accentG;
    float  accentB;
    float  accentA;
    
    // UI scaling
    float  uiScale;
    float  fontScale;
    
    // Platform flags
    bool   nativeDecorations;
    bool   nativeMenuBar;
    
    // Audio
    float  audioVolume;       // v4+
    
    // ImGui blob size (bytes)
    u32    imguiStateSize;
};

// ── Full File Layout ─────────────────────────────────────────────────────────
struct GTKCFile {
    ConfigBinaryData header;
    
    if (header.imguiStateSize > 0) {
        char imguiIniBlob[header.imguiStateSize]; // Opaque ImGui INI payload
    }
};

GTKCFile file @ 0x00;
