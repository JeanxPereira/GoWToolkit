// WadAssetName.cpp
// Parses FileDesc names from God of War Ragnarök WAD files.
//
// Format observed in extracted files:
//   PREFIX_baseName_lodIndex_variant---wadIndex
//
// Known prefixes -> asset type:
//   "MG"          -> GOWR_MESH_GPU  (Mesh Group with GPU buffer, needs lodpack)
//   "ANM"         -> GOWR_ANIM      (Animation clip)
//   "PEM"         -> Particle Emitter
//   "GOWR_SHADER" -> Shader binary  (magic "OrbShdr" @ EOF-28)
//   ""            -> Internal / runtime (DCClientGUID, autopad, PopHeap, ...)
//
// Shaders may also appear as bare hex hashes without a prefix:
//   "a41955a3abdbf109_ps_30000207---2447"
//   -> base = "a41955a3abdbf109", variant = "ps_30000207", wadIndex = 2447

#include "WadTypes.h"
#include <charconv>
#include <array>

// ── Internal helpers ─────────────────────────────────────────────────────────

// Known prefixes. Order matters: longer prefixes first to avoid partial matches
// (e.g. "GOWR_SHADER" before "GOWR_MESH").
static constexpr std::array<std::string_view, 8> kKnownPrefixes = {
    "GOWR_SHADER",
    "GOWR_MESH_DEFN",
    "GOWR_MESH_GPU",
    "GOWR_RIG",
    "GOWR_ANIM",
    "PEM",
    "ANM",
    "MG",
};

// Runtime-generated variant suffixes.
static constexpr std::array<std::string_view, 6> kKnownVariants = {
    "gpu",
    "cpu",
    "ps",   // pixel shader  (may appear as "ps_XXXXXXXX")
    "vs",   // vertex shader
    "cs",   // compute shader
    "inst", // instance override (e.g. "gokratos00_character_lights_overrideInst")
};

static bool IsAllDigits(std::string_view s) {
    if (s.empty()) return false;
    for (char c : s) if (c < '0' || c > '9') return false;
    return true;
}

static bool LooksLikeHexHash(std::string_view s) {
    if (s.size() < 8) return false;
    for (char c : s) {
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) return false;
    }
    return true;
}

// ── WadAssetName::Parse ──────────────────────────────────────────────────────

WadAssetName WadAssetName::Parse(const std::string& raw) {
    WadAssetName result;

    // 1. Separate the stem from the wadIndex using the "---" separator.
    std::string stem = raw;
    {
        auto sep = raw.rfind("---");
        if (sep != std::string::npos) {
            std::string_view idxStr(raw.data() + sep + 3, raw.size() - sep - 3);
            // Strip ".bin" extension if still present in the raw string.
            if (idxStr.size() > 4 && idxStr.substr(idxStr.size() - 4) == ".bin")
                idxStr.remove_suffix(4);
            if (IsAllDigits(idxStr))
                std::from_chars(idxStr.data(), idxStr.data() + idxStr.size(), result.wadIndex);
            stem = raw.substr(0, sep);
        }
        // Strip residual ".bin" from stem if caller passed the full filename.
        if (stem.size() > 4 && stem.substr(stem.size() - 4) == ".bin")
            stem.resize(stem.size() - 4);
    }

    // 2. Detect known prefix.
    for (std::string_view pfx : kKnownPrefixes) {
        if (stem.size() > pfx.size() &&
            stem.compare(0, pfx.size(), pfx) == 0 &&
            stem[pfx.size()] == '_')
        {
            result.prefix = std::string(pfx);
            stem = stem.substr(pfx.size() + 1); // consume "PREFIX_"
            break;
        }
    }

    // 3. Split the remainder into tokens by '_'.
    std::vector<std::string> tokens;
    {
        size_t start = 0;
        while (start < stem.size()) {
            size_t end = stem.find('_', start);
            if (end == std::string::npos) end = stem.size();
            tokens.push_back(stem.substr(start, end - start));
            start = end + 1;
        }
    }

    if (tokens.empty()) {
        result.base = stem;
        return result;
    }

    // 4. First token = baseName.
    result.base = tokens[0];

    // 5. Second token: LOD if it is a short decimal number (0–99).
    size_t next = 1;
    if (next < tokens.size() &&
        IsAllDigits(tokens[next]) && tokens[next].size() <= 2)
    {
        std::from_chars(tokens[next].data(),
                        tokens[next].data() + tokens[next].size(),
                        result.lod);
        next++;
    }

    // 6. Remaining tokens = variant.
    //    Accepted if it starts with a known suffix, or if baseName is a hex hash
    //    (in that case the entire suffix is a shader variant).
    if (next < tokens.size()) {
        bool isKnownVariant = false;
        for (std::string_view v : kKnownVariants) {
            if (tokens[next].size() >= v.size() &&
                tokens[next].compare(0, v.size(), v) == 0)
            {
                isKnownVariant = true;
                break;
            }
        }
        if (!isKnownVariant && LooksLikeHexHash(result.base))
            isKnownVariant = true; // shader hash: entire suffix is variant

        if (isKnownVariant) {
            result.variant = tokens[next];
            for (size_t i = next + 1; i < tokens.size(); i++)
                result.variant += '_' + tokens[i];
        } else {
            // Not a recognized variant: incorporate into baseName to preserve data.
            for (size_t i = next; i < tokens.size(); i++)
                result.base += '_' + tokens[i];
        }
    }

    return result;
}

// ── WadAssetName::CanonicalName ──────────────────────────────────────────────

std::string WadAssetName::CanonicalName() const {
    std::string out;
    if (!prefix.empty()) { out += prefix; out += '_'; }
    out += base;
    if (lod >= 0) { out += '_'; out += std::to_string(lod); }
    if (!variant.empty()) { out += '_'; out += variant; }
    return out;
}
