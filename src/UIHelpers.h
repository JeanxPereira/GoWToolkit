#pragma once
#include "core/AssetDatabase.h"
#include "imgui.h"
#include <string>
#include <sstream>
#include <iomanip>

std::string SystemOpenFileDialog();

// ── Formatação ─────────────────────────────────────────────────────────────

inline std::string HashHex(uint64_t hash) {
    std::ostringstream ss;
    ss << "0x" << std::uppercase << std::hex
       << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

inline std::string FormatBytes(uint64_t bytes) {
    if (bytes < 1024)       return std::to_string(bytes) + " B";
    if (bytes < 1024*1024)  return std::to_string(bytes/1024) + " KB";
    return std::to_string(bytes/(1024*1024)) + " MB";
}

inline std::string FormatNum(uint64_t n) {
    // Insere separador de milhares
    std::string s = std::to_string(n);
    for (int i = (int)s.size()-3; i > 0; i -= 3)
        s.insert(i, ",");
    return s;
}

// ── Tipo → nome / cor ──────────────────────────────────────────────────────

inline const char* TypeName(const std::string& t) {
    if (t == "GOW2_MDL" || t == "GOW1_MDL")  return "MODEL";
    if (t == "GOW2_TXR" || t == "GOW1_TXR")  return "TEXTURE";
    if (t == "GOW2_ANM" || t == "GOW1_ANM")  return "ANIMATION";
    if (t == "GOW2_SFX")                     return "SOUND";
    if (t == "GOW2_VAG")                     return "AUDIO";
    if (t == "GOW2_VPK")                     return "VOICE";
    if (t == "GOW2_WAD_FILE")                return "WAD";
    return t.empty() ? "UNKNOWN" : t.c_str();
}

inline ImVec4 ColorForType(const std::string& t) {
    if (t == "GOW2_MDL" || t == "GOW1_MDL" || t == "GOWR_MESH_DEFN") return {0.4f, 0.8f, 1.0f, 1.0f}; // azul
    if (t == "GOW2_TXR" || t == "GOW1_TXR" || t == "GOWR_TEXTURE")   return {1.0f, 0.5f, 0.8f, 1.0f}; // rosa
    if (t == "GOW2_ANM" || t == "GOW1_ANM")                          return {1.0f, 0.8f, 0.3f, 1.0f}; // amarelo
    if (t == "GOWR_SHADER")                                           return {0.5f, 1.0f, 0.5f, 1.0f}; // verde
    if (t == "GOWR_GOPROTO_RIG" || t == "GOWR_MG_DEFN")              return {1.0f, 0.6f, 0.3f, 1.0f}; // laranja
    if (t == "GOWR_MG_GPU_BUFF")                                      return {0.7f, 0.7f, 1.0f, 1.0f}; // lilás
    if (t == "GOW2_SFX" || t == "GOW2_VAG" || t == "GOW2_VPK")        return {0.3f, 0.9f, 0.6f, 1.0f}; // verde-teal
    if (t == "GOW2_WAD_FILE")                                         return {0.8f, 0.8f, 1.0f, 1.0f}; // azul claro
    if (t == "GOW2_GROUP_START")                                      return {0.9f, 0.9f, 0.9f, 1.0f}; // branco
    return {0.6f, 0.6f, 0.6f, 1.0f}; // cinza
}

inline const char* IconForType(const std::string& t) {
    // GOW1/2 types
    if (t == "GOW2_MDL" || t == "GOW1_MDL")  return (const char*)"\xEE\xA8\xBB"; // codicon: symbol-misc (cube)
    if (t == "GOW2_TXR" || t == "GOW1_TXR")  return (const char*)"\xEE\xA8\xBB"; // codicon: file-media
    if (t == "GOW2_ANM" || t == "GOW1_ANM")  return (const char*)"\xEE\xA8\xBB"; // codicon: play
    if (t == "GOW2_SFX" || t == "GOW2_VAG" || t == "GOW2_VPK")
                                             return (const char*)"\xEE\xA8\xBB"; // codicon: sound
    if (t == "GOW2_WAD_FILE")                return (const char*)"\xEE\xA9\x83"; // codicon: folder
    if (t == "GOW2_GROUP_START")              return (const char*)"\xEE\xA9\x83"; // codicon: folder
    // GOWR types
    if (t == "GOWR_MESH_DEFN" || t == "GOWR_MG_DEFN") return (const char*)"\xEE\xA8\xBB"; // cube
    if (t == "GOWR_TEXTURE")                  return (const char*)"\xEE\xA8\xBB"; // image
    if (t == "GOWR_SHADER")                   return (const char*)"\xEE\xA8\xBB"; // code
    if (t == "GOWR_GOPROTO_RIG")              return (const char*)"\xEE\xA8\xBB"; // person
    // Default: generic file icon
    return (const char*)"\xEE\xA9\xBB"; // codicon: file
}

inline const char* SkinModeName(uint8_t mode) {
    switch (mode) {
        case 1:  return "4-8 joints (R10G10B10A2)";
        case 2:  return "7 joints (R16)";
        case 3:  return "10 joints (packed)";
        default: return "unknown";
    }
}

inline bool MatchesFilter(const std::string& name, const char* filter) {
    if (!filter || !filter[0]) return true;
    std::string n = name, f = filter;
    // case-insensitive
    for (auto& c : n) c = (char)tolower(c);
    for (auto& c : f) c = (char)tolower(c);
    return n.find(f) != std::string::npos;
}
