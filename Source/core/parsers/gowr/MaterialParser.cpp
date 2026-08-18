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
    // TX_<subject>_gen_0<suffix>_<16 hex>
    const size_t last = name.find_last_of('_');
    if (last == std::string::npos || last < 2) return TextureRole::Unknown;
    const size_t prev = name.find_last_of('_', last - 1);
    if (prev == std::string::npos) return TextureRole::Unknown;

    std::string tag = name.substr(prev + 1, last - prev - 1);
    if (tag.size() < 2 || tag[0] != '0') return TextureRole::Unknown;
    tag.erase(0, 1);

    if (tag == "d")  return TextureRole::Diffuse;
    if (tag == "n")  return TextureRole::Normal;
    if (tag == "ao") return TextureRole::AmbientOcclusion;
    if (tag == "h")  return TextureRole::Height;
    if (tag == "s")  return TextureRole::Specular;
    if (tag == "r")  return TextureRole::Roughness;
    if (tag == "m")  return TextureRole::Metallic;
    if (tag == "e")  return TextureRole::Emissive;
    if (tag == "g")  return TextureRole::Gloss;
    if (tag == "sc") return TextureRole::Scatter;
    if (tag == "sd") return TextureRole::Detail;
    return TextureRole::Unknown;
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
        default:                            return "Unknown";
    }
}

const MatReference* GOWRMaterial::Texture(TextureRole role) const {
    for (const auto& r : refs)
        if (r.isTexture && r.role == role) return &r;
    return nullptr;
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
    if (count == 0 || count > 4096) {
        LOG_WARN("[GOWRMaterial] implausible reference count %u", count);
        return false;
    }

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
            // The hash is stored as two big-endian-printed u32s, which is the
            // order the name spells it in; keep that order so it matches the
            // texpack key.
            uint32_t lo = 0, hi = 0;
            std::memcpy(&lo, ref.guid + 8,  4);
            std::memcpy(&hi, ref.guid + 12, 4);
            ref.textureHash = (static_cast<uint64_t>(lo) << 32) | hi;
            ref.role        = RoleFromName(ref.name);
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
        LOG_WARN("[GOWRMaterial] entry is only %zu bytes", size);
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
        LOG_WARN("[GOWRMaterial] parameter table (%u entries at 0x%X) does not "
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

    LOG_INFO("[GOWRMaterial] %016llX: %zu params, %zu textures, %zu shaders",
             (unsigned long long)out.hash, out.params.size(),
             out.Textures().size(), out.Shaders().size());
    return true;
}

} // namespace Onyx
