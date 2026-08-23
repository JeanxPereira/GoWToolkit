#include "MaterialParser.h"
#include <Onyx/Services/Logger.h>
#include <cctype>
#include <cstring>

// -- MaterialParser.cpp -------------------------------------------------------
// Layout established by decoding r_athena00's MAT_DE674F96622453EB and its
// companion. See docs/GoWRknk/Formats/Material.md.
//
// MAT entry:
//   +0x00  u32   version tag (0x0A observed)
//   +0x0C  u32   pipeline flags, matching the shader header's field
//   +0x10  u64   the material's own hash, the one its shaders are named after
//   +0xC0  u32[] section offsets, from the start of the entry
//   +0xE0  u16[] section element counts
//
//   Section 0 is the parameter table, 24 bytes per entry.
//
// Companion entry (type 0, follows the MAT):
//   +0x00  u32   reference count
//   +0x04  u32   (0)
//   +0x08  entries, 76 bytes each: a 16-byte asset GUID then a 60-byte name.
//
// A GUID is four u32 words. A texture GUID spells "TEXT" "URE" in its first
// two words, but only read big-endian: on disk the bytes are swapped within
// each word, so the tag is not the ASCII string. The last two words are the
// texture hash, in the order the name prints it.

namespace Onyx {

namespace {

constexpr size_t kRefStride    = 76;
constexpr size_t kRefNameBytes = kRefStride - 16;
constexpr size_t kParamStride  = 24;

// Compared as bytes, not as the string it spells; see the note above.
const uint8_t kTextureTag[8] = { 0x54, 0x58, 0x45, 0x54, 0x00, 0x45, 0x52, 0x55 };

TextureRole RoleFromName(const std::string& name) {
    // Two conventions are in use, and both must be read:
    //   TX_<subject>_gen_0<tag>_<hash>   1223 textures across four character WADs
    //   TX_<subject>_<tag>_<hash>          ~700 more
    // Taking only the first form leaves materials looking like they declare no
    // diffuse at all, when in fact they name one as _d_ rather than _0d_.
    const size_t last = name.find_last_of('_');
    if (last == std::string::npos || last < 2) return TextureRole::Unknown;
    const size_t prev = name.find_last_of('_', last - 1);
    if (prev == std::string::npos) return TextureRole::Unknown;

    std::string tag = name.substr(prev + 1, last - prev - 1);
    if (tag.empty()) return TextureRole::Unknown;

    // "gen" is a literal word in most asset names, not a channel, and it sits in
    // exactly the position a tag occupies when a name carries no channel at all.
    if (tag == "gen" || tag == "ge") return TextureRole::Unknown;

    // A channel tag can be wrapped in digits on both sides:
    //
    //   _gen_0d_   leading digit  = the layer this map belongs to
    //   _hair_d2_  trailing digit = a variant index
    //
    // Only a leading '0' used to be stripped, so every variant read as an
    // unknown channel and was dropped. That is why Baldur's hair and beard
    // rendered white: their diffuse is named _hair_d2_ and _hair_d4_, their
    // normal _hair_nm2_, and all three were discarded.
    while (tag.size() > 1 && tag.front() >= '0' && tag.front() <= '9') tag.erase(0, 1);

    // "m<N>" is the dynamicmaterial region index (m1/m2/m3 alongside a
    // regionidmap), not a metallic map. Stripping its digit would read it as
    // one, and no GOWR asset seen so far names a metallic map at all -- the
    // canonical channel set is d/n/ao/g/h/e/sc/sd.
    const bool regionIndex = tag.size() > 1 && tag[0] == 'm' &&
                             tag.find_first_not_of("0123456789", 1) == std::string::npos;
    if (regionIndex) return TextureRole::Unknown;

    while (tag.size() > 1 && tag.back() >= '0' && tag.back() <= '9') tag.pop_back();

    if (tag == "d")  return TextureRole::Diffuse;
    if (tag == "n" || tag == "nm") return TextureRole::Normal;
    if (tag == "ao" || tag == "o")  return TextureRole::AmbientOcclusion;
    if (tag == "h")  return TextureRole::Height;
    if (tag == "s")  return TextureRole::Specular;
    if (tag == "r")  return TextureRole::Roughness;
    if (tag == "m")  return TextureRole::Metallic;
    if (tag == "e")  return TextureRole::Emissive;
    if (tag == "g")  return TextureRole::Gloss;
    if (tag == "sc") return TextureRole::Scatter;
    if (tag == "sd") return TextureRole::Detail;
    // Coverage lives in its own map here rather than in the diffuse alpha,
    // which is why hair and a cornea rendered as solid geometry: the diffuse
    // they ship is fully opaque, so the shader's cutout had nothing to act on.
    if (tag == "opc" || tag == "op" || tag == "alpha" || tag == "opacity")
        return TextureRole::Opacity;
    return TextureRole::Unknown;
}

// The trailing 16 hex digits of an asset name are its hash, printed as two
// big-endian words - the same order the texpack index is keyed by.
bool HashFromName(const std::string& name, uint64_t& out) {
    if (name.size() < 17 || name[name.size() - 17] != '_') return false;

    uint64_t v = 0;
    for (size_t i = name.size() - 16; i < name.size(); ++i) {
        const char c = name[i];
        uint64_t d;
        if      (c >= '0' && c <= '9') d = (uint64_t)(c - '0');
        else if (c >= 'A' && c <= 'F') d = (uint64_t)(c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') d = (uint64_t)(c - 'a' + 10);
        else return false;
        v = (v << 4) | d;
    }
    out = v;
    return true;
}

bool LooksLikeShaderName(const std::string& n) {
    return n.find("_vs_") != std::string::npos ||
           n.find("_ps_") != std::string::npos ||
           n.find("_cs_") != std::string::npos;
}

} // namespace

const char* TextureRoleName(TextureRole role) {
    switch (role) {
        case TextureRole::Diffuse:          return "Diffuse";
        case TextureRole::Normal:           return "Normal";
        case TextureRole::AmbientOcclusion: return "Ambient Occlusion";
        case TextureRole::Height:           return "Height";
        case TextureRole::Specular:         return "Specular";
        case TextureRole::Roughness:        return "Roughness";
        case TextureRole::Metallic:         return "Metallic";
        case TextureRole::Emissive:         return "Emissive";
        case TextureRole::Gloss:            return "Gloss";
        case TextureRole::Scatter:          return "Scatter";
        case TextureRole::Detail:           return "Detail";
        case TextureRole::Opacity:          return "Opacity";
        default:                            return "Unknown";
    }
}

const MatReference* GOWRMaterial::Texture(TextureRole role) const {
    // Two naming conventions carry a channel, and they are not equivalent:
    //
    //   TX_<subject>_gen_0<tag>_<hash>   the subject's own map for that channel
    //   TX_<something>_<tag>_<hash>      often a SHARED map, not this subject's
    //
    // Returning the first match regardless bound TX_wave_flow_n to the normal
    // slot of Baldur's head, arms, chest, legs and lower body -- one shared FX
    // map standing in for five different subjects -- because it happened to sit
    // earlier in a reference list of 39. The canonical form wins when the
    // material declares one.
    //
    // A preference, not a filter: TX_baldur00_beard_d is the beard's real
    // diffuse and uses the bare form, so excluding it would lose the texture
    // rather than improve it.
    const MatReference* fallback = nullptr;
    for (const auto& r : refs) {
        if (!r.isTexture || r.role != role) continue;
        if (r.name.find("_gen_0") != std::string::npos) return &r;
        if (!fallback) fallback = &r;
    }
    return fallback;
}

std::vector<const MatReference*> GOWRMaterial::Textures() const {
    std::vector<const MatReference*> out;
    for (const auto& r : refs) if (r.isTexture) out.push_back(&r);
    return out;
}

std::vector<const MatReference*> GOWRMaterial::Shaders() const {
    std::vector<const MatReference*> out;
    for (const auto& r : refs) if (r.isShader) out.push_back(&r);
    return out;
}

bool GOWRMaterialParseRefs(const std::shared_ptr<Vfs::IFile>& file,
                           std::vector<MatReference>& out)
{
    out.clear();
    if (!file || !file->IsValid()) return false;

    const size_t size = file->Size();
    if (size < 8 + kRefStride) return false;

    file->Seek(0, SEEK_SET);
    uint32_t count = 0;
    file->Read(&count, 4);
    // No warning here: this function doubles as the probe that decides whether
    // an arbitrary entry is a reference list at all, so an implausible count is
    // the expected answer for most entries rather than a problem.
    if (count == 0 || count > 4096) return false;

    out.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const size_t off = 8 + (size_t)i * kRefStride;
        if (off + 16 > size) break;   // the last name field can be truncated

        MatReference ref;
        file->Seek(off, SEEK_SET);
        file->Read(ref.guid, 16);

        char name[kRefNameBytes + 1] = {};
        const size_t avail = (off + kRefStride <= size) ? kRefNameBytes
                                                        : (size - off - 16);
        file->Read(name, avail);
        name[avail] = '\0';
        ref.name = name;

        if (std::memcmp(ref.guid, kTextureTag, 8) == 0) {
            ref.isTexture = true;
            ref.role      = RoleFromName(ref.name);

            // The texpack is keyed by the hash the asset NAME ends in, and
            // that is not always what the GUID carries: across r_heroa00's
            // 8396 texture references the two disagree 937 times. Measured
            // on the same WAD, 2772 of 2798 name hashes resolve in a
            // texpack, so the name is the reliable key and the GUID is only
            // a fallback for a reference that carries no hash in its name.
            uint32_t lo = 0, hi = 0;
            std::memcpy(&lo, ref.guid + 8,  4);
            std::memcpy(&hi, ref.guid + 12, 4);
            ref.textureHash = (static_cast<uint64_t>(lo) << 32) | hi;

            uint64_t fromName = 0;
            if (HashFromName(ref.name, fromName)) ref.textureHash = fromName;
        } else if (LooksLikeShaderName(ref.name)) {
            ref.isShader = true;
        }

        out.push_back(std::move(ref));
    }

    return !out.empty();
}

bool GOWRMaterialParse(const std::shared_ptr<Vfs::IFile>& mat,
                       const std::shared_ptr<Vfs::IFile>& refList,
                       GOWRMaterial& out)
{
    out = GOWRMaterial{};
    if (!mat || !mat->IsValid()) return false;

    const size_t size = mat->Size();
    if (size < 0xF0) {
        ONYX_LOGF_WARN("[GOWRMaterial] entry is only %zu bytes", size);
        return false;
    }

    mat->Seek(0x10, SEEK_SET);
    mat->Read(&out.hash, 8);

    uint32_t sectionOffsets[7] = {};
    mat->Seek(0xC0, SEEK_SET);
    mat->Read(sectionOffsets, sizeof(sectionOffsets));

    uint16_t sectionCounts[6] = {};
    mat->Seek(0xE0, SEEK_SET);
    mat->Read(sectionCounts, sizeof(sectionCounts));

    const uint32_t paramsAt    = sectionOffsets[0];
    const uint16_t paramCount  = sectionCounts[0];
    if (paramsAt >= size ||
        paramsAt + (size_t)paramCount * kParamStride > size) {
        ONYX_LOGF_WARN("[GOWRMaterial] parameter table (%u entries at 0x%X) does not "
                 "fit in %zu bytes", paramCount, paramsAt, size);
    } else {
        out.params.reserve(paramCount);
        mat->Seek(paramsAt, SEEK_SET);
        for (uint16_t i = 0; i < paramCount; ++i) {
            MatParam p;
            mat->Read(&p.nameHash, 8);
            mat->Read(&p.typeHash, 8);
            mat->Read(&p.typeCode, 2);
            mat->Read(&p.cbufferOffset, 2);
            mat->Read(&p.value, 4);
            out.params.push_back(p);
        }
    }

    if (refList) GOWRMaterialParseRefs(refList, out.refs);

    ONYX_LOGF_INFO("[GOWRMaterial] %016llX: %zu params, %zu textures, %zu shaders",
             (unsigned long long)out.hash, out.params.size(),
             out.Textures().size(), out.Shaders().size());
    return true;
}

} // namespace Onyx
