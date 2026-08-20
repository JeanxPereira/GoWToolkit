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
#include "core/profiles/gowr/GowrTaxonomy.h"
#include <Onyx/Domain/Entry.h>
#include "core/types/GameTypes.h"
#include <charconv>
#include <array>
#include <cctype>
#include <vector>

namespace Onyx {
namespace Gowr {

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

// ── ClassifyByName ───────────────────────────────────────────────────────
// Priority-ordered pattern matching. Moved verbatim from
// WadNodeBuilder::ClassifyByName (formerly private to the builder) so this
// is the single copy both Pass1_Classify and Classify() below call — the
// two must never be allowed to drift apart.

WadEntryRole ClassifyByName(const std::string& name, uint64_t size) {
    if (name.empty()) return WadEntryRole::Unknown;

    // ── Sentinels ────────────────────────────────────────────────────────
    if (name == "PopHeap" || name == "autopad")
        return WadEntryRole::Sentinel;

    // ── SharedWadRef: ^[A-Z]+X_R_  (e.g. TXRX_R_Fox00, ANMX_R_Fox00) ──
    // Must be checked BEFORE WadIdentity to avoid matching WAD_R_ (no X before _R_)
    {
        size_t i = 0;
        while (i < name.size() && isupper((unsigned char)name[i])) ++i;
        // i >= 2 ensures at least 2 uppercase chars; name[i-1] must be 'X'
        if (i >= 2 && name[i - 1] == 'X' &&
            name.size() > i + 2 &&
            name[i] == '_' && name[i + 1] == 'R' && name[i + 2] == '_')
        {
            return WadEntryRole::SharedWadRef;
        }
    }

    // ── WAD identity: starts with WAD_ ───────────────────────────────────
    if (name.rfind("WAD_", 0) == 0)
        return WadEntryRole::WadIdentity;

    // ── Shader container: starts with 0x ─────────────────────────────────
    if (name.rfind("0x", 0) == 0)
        return WadEntryRole::ShaderContainer;

    // ── Shaders: _vs_ / _ps_ anywhere in name ────────────────────────────
    if (name.find("_vs_") != std::string::npos)
        return WadEntryRole::ShaderVertex;
    if (name.find("_ps_") != std::string::npos)
        return WadEntryRole::ShaderPixel;
    if (name.find("_hs_") != std::string::npos)
        return WadEntryRole::ShaderHull;
    if (name.find("_ds_") != std::string::npos)
        return WadEntryRole::ShaderDomain;
    if (name.find("_cs_") != std::string::npos)
        return WadEntryRole::ShaderCompute;
    if (name.find("_ls_") != std::string::npos)
        return WadEntryRole::ShaderLibrary;

    // Named vertex shaders without hash prefix
    if (name.rfind("depth_vs",  0) == 0 ||
        name.rfind("depvl_vs",  0) == 0 ||
        name.rfind("opaque_vs", 0) == 0 ||
        name.rfind("transp_vs", 0) == 0)
    {
        return WadEntryRole::ShaderVertex;
    }

    // ── Animation ────────────────────────────────────────────────────────
    if (name.rfind("ANM_", 0) == 0)
        return WadEntryRole::AnimClip;

    // ── Textures (discriminated by size) ─────────────────────────────────
    if (name.rfind("TX_", 0) == 0)
        return (size >= 1024) ? WadEntryRole::TextureGpu : WadEntryRole::TextureCpu;

    // ── Materials (discriminated by size) ────────────────────────────────
    if (name.rfind("MAT_", 0) == 0)
        return (size > 0) ? WadEntryRole::Material : WadEntryRole::MaterialRef;

    // ── LOD binding table: matches /^\d+_\d+_\d+$/ ───────────────────────
    {
        bool isLod       = !name.empty();
        int  underscores = 0;
        for (char c : name) {
            if      (c == '_')             ++underscores;
            else if (!isdigit((unsigned char)c)) { isLod = false; break; }
        }
        if (isLod && underscores == 2)
            return WadEntryRole::LodBinding;
    }

    // ── Mesh (order matters: MeshGpu before MeshDefn) ────────────────────
    if (name.rfind("MG_", 0) == 0) {
        if (name.size() > 4 && name.substr(name.size() - 4) == "_gpu")
            return WadEntryRole::MeshGpu;
        return WadEntryRole::MeshDefn;   // MG_ without _gpu = mesh group def
    }
    if (name.rfind("MESH_", 0) == 0)
        return WadEntryRole::MeshDefn;
    if (name.rfind("MDL_", 0) == 0)
        return WadEntryRole::Model;

    // ── Game objects (order matters: Override > Proto > Inst) ────────────
    if (name.rfind("goProto", 0) == 0)
        return WadEntryRole::GameObjectProto;

    if (name.size() >= 13 &&
        name.substr(name.size() - 13) == "_overrideInst")
    {
        return WadEntryRole::GameObjectOverride;
    }

    // Plain go* instance: starts with "go" followed by a lowercase letter
    // (guards against matching "goProto" again and other non-game-object "go" names)
    if (name.size() > 2 &&
        name[0] == 'g' && name[1] == 'o' &&
        islower((unsigned char)name[2]))
    {
        return WadEntryRole::GameObjectInst;
    }

    // ── Audio ─────────────────────────────────────────────────────────────
    if (name.rfind("SEMW_", 0) == 0)
        return WadEntryRole::SoundEmitter;

    // ── Particle FX ───────────────────────────────────────────────────────
    if (name == "DCClientGUID")
        return WadEntryRole::ClientGuid;
    if (name.rfind("PEM_emit_", 0) == 0)
        return WadEntryRole::ParticleEmitter;
    if (name.rfind("PTC_part_", 0) == 0)
        return WadEntryRole::ParticleSystem;

    return WadEntryRole::Unknown;
}

// ── Classify ──────────────────────────────────────────────────────────────
// Reconstructs the tag AssetEntry::profileTag used to carry, from the entry
// alone. `parsedName` is a pure function of the name; `block` is deliberately
// not derived (see GowrTaxonomy.h).
//
// `role` is name+size classification EXCEPT for textures, and that exception is
// load-bearing. ClassifyByName splits TX_* on size (>= 1024 => TextureGpu, else
// TextureCpu) because that is how the raw FileDesc stream is classified during
// the parse walk. But no raw texture entry ever reaches the tree: the builder
// consumes each TextureCpu into its matching TextureGpu ("CPU will become a
// child of GPU") and emits ONE node for the pair, which the pre-port code then
// relabelled TexturePair by overwriting the stored tag.
//
// So a tree node typed TexturePair is always that synthesised pair node, and
// classifying it by size would report TextureGpu -- the very label the builder
// overwrote. Trust the typeId here; it is the only place the two disagree.
GowrProfileTag Classify(const Onyx::Domain::AssetEntry& entry) {
    GowrProfileTag tag;
    tag.parsedName = WadAssetName::Parse(entry.name);
    tag.role       = (entry.typeId == GameTypes::TexturePair)
                        ? WadEntryRole::TexturePair
                        : ClassifyByName(entry.name, entry.source.size);
    return tag;
}

} // namespace Gowr
} // namespace Onyx
