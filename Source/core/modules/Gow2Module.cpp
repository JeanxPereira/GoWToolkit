#include "Gow2Module.h"

#include <Onyx/Modules/Workspace.h>
#include <Onyx/Vfs/IsoFileSystem.h>
#include <Onyx/Vfs/SliceFile.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Fonts/SFSymbols.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/types/WadDispatch.h"
#include "core/types/GameTypes.h"
#include "core/parsers/gow2/TextureParser.h"

// ── Gow2Module.cpp ─────────────────────────────────────────────────────────
// Ported from ProfileGOW2.{h,cpp} (core/profiles/gow2/). Two independent
// bodies, matching the two things the old profile's Detect()/MountArchive()/
// LoadFromArchive() dispatched between:
//
//   - ParseWadTagStream: ProfileGOW2::ParseContainer's 32-byte RawWadTag
//     walker, ported verbatim onto ctx.file / ctx.roots / ctx.diags. Runs
//     when Mounts() did not match (a bare .wad).
//   - ParseIsoToc: ProfileGOW2::LoadFromArchive + LoadFromArchiveGOW2's
//     GOW2.TOC/GODOFWAR.TOC walk, ported onto ctx.mountedVfs. Runs when the
//     .iso MountSpec below mounted successfully. THIS is the first live use
//     of ByteRange::fileIndex in the app: every PART*.PAK the TOC references
//     is opened once through ctx.mountedVfs, pushed into ctx.fileTable, and
//     every entry addressing bytes inside that PAK is stamped with the
//     index that push_back returned -- never a guessed constant. See
//     task-3-report.md for how this was verified against every entry, not
//     only the ones touched by hand.
//
// Both bodies end by running FinalizeEntries (below), which VALIDATES --
// it does NOT stamp -- source.fileIndex tree-wide, flagging (but keeping in
// the tree) any entry whose computed byte range runs past the end of the
// file it addresses (salvage, task-3-brief.md Decision 4). This is a
// narrower contract than GowrModule.cpp's FinalizeEntries (Task 2), which
// genuinely stamps fileIndex tree-wide because GOWR's tree can have folders
// synthesised after the fact by WadNodeBuilder. GOW2's ISO path builds a
// flat list with fileIndex already set correctly at each entry's
// construction site (see ParseIsoToc), and its WAD path never pushes
// anything beyond fileTable's pre-seeded slot 0 -- so there is nothing left
// for this function to stamp. A future GOW2 path that pushes a file into
// fileTable MUST stamp source.fileIndex itself; this function will not do
// it.

namespace Onyx::Gow2 {

using Onyx::Domain::MediaKind;
using Onyx::Domain::NodeFlags;
using Onyx::Modules::ContainerContext;
using Onyx::Modules::DecodeContext;
using Onyx::Modules::DecoderRegistry;
using Onyx::Modules::ModuleInfo;
using Onyx::Modules::MountSpec;
using Onyx::Modules::OpenFilter;
using Onyx::Modules::ParseResult;
using Onyx::Modules::ProbeInput;
using Onyx::Modules::ProbeResult;
using Onyx::Services::Diag;
using Onyx::Services::DiagSink;
using Onyx::Services::Severity;
using AssetEntryT = Onyx::Domain::AssetEntry;

// ═══════════════════════════════════════════════════════════════════════════
// Wire structs — GOW2 WAD tag stream (RawWadTag) and ISO TOC (RawTocEntryGOW2)
// Defined once, up top, so both Probe() (needs RawWadTag to sniff the header)
// and the two ParseContainer bodies can see them. Ported verbatim from
// ProfileGOW2.cpp (they were file-local there too, just declared mid-file).
// ═══════════════════════════════════════════════════════════════════════════

#pragma pack(push, 1)
struct RawWadTag {
    uint16_t tag;
    uint16_t flags;
    uint32_t size;
    char name[24];
};
#pragma pack(pop)

// Tag numbers from GOW2 WAD format (matches reference god_of_war_browser/pack/wad/gow2.go)
static constexpr uint16_t WADTAG_ENTITY_COUNT  = 0;
static constexpr uint16_t WADTAG_SERVER_INST   = 1;
static constexpr uint16_t WADTAG_GROUP_START   = 2;
static constexpr uint16_t WADTAG_GROUP_END     = 3;
static constexpr uint16_t WADTAG_HEADER_POP    = 19;
static constexpr uint16_t WADTAG_HEADER_START  = 21;
// Tags 11-16 are TT_* (tweak template) nodes — added as leaves, no group semantics

#pragma pack(push, 1)
struct RawTocEntryGOW2 {
    char     name[24];
    uint32_t size;
    uint32_t encountersCount;
    uint32_t encountersStart;
};
#pragma pack(pop)

// ═══════════════════════════════════════════════════════════════════════════
// Info / Probe
// ═══════════════════════════════════════════════════════════════════════════

ModuleInfo Gow2Module::Info() const {
    return ModuleInfo{
        "gow2",
        "God of War I / II (PS2)",
        {"gow2", "gow1", "ps2"},
        {OpenFilter{"God of War I / II (PS2)", {"iso", "wad"}}},
    };
}

// Evidence-scored per spec (docs/superpowers/specs/2026-08-20-port-phase-2-
// game-modules-design.md, Decision 1): 90 for .iso, 80 for a readable GOW2
// tag stream actually seen at the start of the supplied header bytes, 45 for
// a bare .wad with neither. The old ProfileGOW2::Detect's "unless it sniffs
// GOWR magic" exclusion is gone -- under RankProbes (>= 40 AND strictly
// beats the runner-up), GOWR's 95-for-real-evidence simply outscores GOW2's
// 45-for-a-bare-.wad without either module needing to know the other
// exists. Pure: only path/header/fileSize, no I/O; bounds-checked against a
// header shorter than sizeof(RawWadTag) (the host promises up to 64 KiB, not
// exactly 64 KiB -- a truncated real file can hand Probe() less).
ProbeResult Gow2Module::Probe(const ProbeInput& in) const {
    std::string ext = in.path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return char(std::tolower(c)); });

    if (ext == ".iso") {
        return ProbeResult{90, "ISO extension"};
    }

    if (in.header.size() >= sizeof(RawWadTag)) {
        RawWadTag t{};
        std::memcpy(&t, in.header.data(), sizeof(RawWadTag));

        // "Readable" means: the first tag is one of the small set of
        // structural/leaf tags the walker below actually understands, and
        // its declared payload size does not already overrun the file --
        // the same class of sanity check LoadFromArchive's isGOW2 check
        // applies to the TOC header, just for the tag stream instead.
        //
        // Excludes the all-zero record (tag=0, flags=0, size=0) explicitly:
        // WADTAG_ENTITY_COUNT is 0, so a naive `tag <= WADTAG_GROUP_END`
        // check alone would score 80 for ANY file whose first 8 bytes
        // happen to be zero -- not evidence of a GOW2 tag stream, just an
        // empty/zeroed header many unrelated formats could have.
        bool allZeroHeader = (t.tag == 0 && t.flags == 0 && t.size == 0);
        bool knownTag = !allZeroHeader &&
                        ((t.tag <= WADTAG_GROUP_END) ||
                         (t.tag >= 11 && t.tag <= 16) ||
                         t.tag == WADTAG_HEADER_POP ||
                         t.tag == WADTAG_HEADER_START);

        // RankProbes' ProbeInput overload lets a caller leave fileSize == 0
        // (unknown), which must NOT be read as "the file is 0 bytes, so any
        // positive tag size overruns it" -- that would silently demote a
        // genuine GOW2 wad from 80 to 45 just because the caller didn't
        // populate fileSize. Only fail the check when fileSize is actually
        // known and the declared payload provably overruns it.
        bool sizeSane = (in.fileSize == 0) ||
                        ((uint64_t)sizeof(RawWadTag) + t.size <= in.fileSize);

        if (knownTag && sizeSane) {
            return ProbeResult{80, "readable GOW2 tag stream in header"};
        }
    }

    if (ext == ".wad") {
        return ProbeResult{45, "bare .wad extension"};
    }

    return ProbeResult{0, "no evidence"};
}

// ═══════════════════════════════════════════════════════════════════════════
// Mounts — the ISO becomes a real MountSpec
// ═══════════════════════════════════════════════════════════════════════════

std::vector<MountSpec> Gow2Module::Mounts() const {
    return {
        MountSpec{
            "God of War I / II (PS2 ISO)",
            {"iso"},
            [](const std::filesystem::path& path) -> std::shared_ptr<Vfs::IVirtualFileSystem> {
                auto vfs = std::make_shared<Vfs::IsoFileSystem>(path.string());
                if (!vfs->Initialize()) {
                    return nullptr;
                }
                ONYX_LOGF_INFO("[GOW2] Successfully mounted ISO: %s", path.string().c_str());
                return vfs;
            },
        },
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// RegisterTypes
// ═══════════════════════════════════════════════════════════════════════════

// Mints one module-scoped TypeId per row 0-26 of core/types/GameTypeTable.h
// -- every type either ParseWadTagStream's handler/fallback resolution or
// ParseIsoToc's AssignSchemaType can stamp onto a GOW2 entry (cross-checked
// against both functions below and against GameTypes.h's extern list, which
// is exactly rows 0-26; rows 27+ are GOWR's Shader family, unreachable from
// this module). Keys are bare per TypeRegistrar::Add's contract, lower-
// camel-cased from the corresponding Onyx::GameTypes::* extern name; label/
// media/icon/color match the table row-for-row.
void Gow2Module::RegisterTypes(Onyx::Types::TypeRegistrar& r) {
    auto add = [&r](const char* key, const char* label, MediaKind media,
                     const char* icon, float cr, float cg, float cb, float ca) {
        Onyx::Types::TypeInfo info;
        info.key   = key;
        info.label = label;
        info.media = media;
        info.icon  = icon;
        info.color[0] = cr; info.color[1] = cg; info.color[2] = cb; info.color[3] = ca;
        return r.Add(info);
    };

    m_unknown     = add("unknown",     "Unknown",      MediaKind::Unknown,   ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_entityCount = add("entityCount", "Entity Count", MediaKind::Unknown,   ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_groupStart  = add("groupStart",  "Group Start",  MediaKind::Unknown,   ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_groupEnd    = add("groupEnd",    "Group End",    MediaKind::Unknown,   ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_headerStart = add("headerStart", "Header Start", MediaKind::Unknown,   ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_headerPop   = add("headerPop",   "Header Pop",   MediaKind::Unknown,   ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_instance    = add("instance",    "Instance",     MediaKind::Map,       ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_object      = add("object",      "Object",       MediaKind::Skeleton,  ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_model       = add("model",       "Model",        MediaKind::Mesh,      ICON_SF_CUBE_FILL, 0.4f, 0.8f, 1.0f, 1.0f);
    m_mesh        = add("mesh",        "Mesh",         MediaKind::Mesh,      ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_material    = add("material",    "Material",     MediaKind::Material,  ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_texture     = add("texture",     "Texture",      MediaKind::Image,     ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_gfxData     = add("gfxData",     "GFX Data",     MediaKind::Raw,       ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_palData     = add("palData",     "Palette",      MediaKind::Raw,       ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_animation   = add("animation",   "Animation",    MediaKind::Animation, ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_script      = add("script",      "Script",       MediaKind::Script,    ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_light       = add("light",       "Light",        MediaKind::Raw,       ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_sound       = add("sound",       "Sound",        MediaKind::Audio,     ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_collision   = add("collision",   "Collision",    MediaKind::Raw,       ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_flipbook    = add("flipbook",    "Flipbook",     MediaKind::Raw,       ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_chunk       = add("chunk",       "Chunk",        MediaKind::Raw,       ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_wadFile     = add("wadFile",     "WAD File",     MediaKind::Container, ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_vagAudio    = add("vagAudio",    "VAG Audio",    MediaKind::Audio,     ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_vpkVideo    = add("vpkVideo",    "VPK Video",    MediaKind::Video,     ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_pssVideo    = add("pssVideo",    "PSS Video",    MediaKind::Video,     ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_pswVideo    = add("pswVideo",    "PSW Video",    MediaKind::Video,     ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
    m_textPlain   = add("textPlain",   "Text",         MediaKind::Script,    ICON_SF_DOCUMENT, 0.6f, 0.6f, 0.6f, 1.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// RegisterDecoders
// ═══════════════════════════════════════════════════════════════════════════

void Gow2Module::RegisterDecoders(DecoderRegistry& reg) {
    // Every decoder is registered TWICE, under both ids the same asset can
    // carry.
    //
    // ParseWadTagStream stamps a handler's legacy Onyx::GameTypes::* id
    // whenever WadTypeRegistry::ResolveByTag finds one, and this module's own
    // minted id when it does not (see the comment at that call site). The
    // DecoderRegistry keys on the id an entry actually carries, so
    // registering only the minted one left every handler-resolved entry
    // without a decoder -- `decode <wad> TXR_boar:file7` answered "no decoder
    // for entry" on a texture whose decoder was right here, because the entry
    // was typed GOW2_TEXTURE and this line said gow2.texture.
    //
    // Converging the two id families is the real fix and is recorded on
    // Gow2Module.h's member list; registering both keys is what makes decode
    // work in the meantime, and costs one map insert per type. Legacy ids are
    // only registered when valid: GameTypes::RegisterGameTypes() may not have
    // run (gowtoolkit_tests does not always call it), and an invalid handle
    // must not become a registry key.
    auto both = [&](Onyx::Types::TypeId minted, Onyx::Types::TypeId legacy, auto slot) {
        slot(minted);
        if (legacy.valid() && legacy != minted) slot(legacy);
    };

    // TextPlain is self-contained (see DecodeTextPlain) -- implemented.
    both(m_textPlain, Onyx::GameTypes::TextPlain,
         [&](Onyx::Types::TypeId id) { reg.Text(id, &Gow2Module::DecodeTextPlain); });

    // Model/Mesh/Object need per-part material resolution (Phase 3 scope,
    // see Gow2Module.h) -- stubbed rather than left unregistered, so a
    // caller gets an explicit diag instead of DecoderRegistry silently
    // reporting "no decoder".
    auto scene = [&](Onyx::Types::TypeId id) { reg.Scene(id, &Gow2Module::DecodeSceneStub); };
    both(m_model,  Onyx::GameTypes::Model,  scene);
    both(m_mesh,   Onyx::GameTypes::Mesh,   scene);
    both(m_object, Onyx::GameTypes::Object, scene);

    // Texture is implemented: GOW2TextureParser::Parse's sibling-resolution
    // need is satisfied by ctx.doc.roots (see DecodeImage below).
    both(m_texture, Onyx::GameTypes::Texture,
         [&](Onyx::Types::TypeId id) { reg.Image(id, &Gow2Module::DecodeImage); });

    // No decoder slot fits Material/Instance (Map)/GfxData/PalData/
    // Animation/Script/Light/Sound/Collision/Flipbook/Chunk/WadFile/
    // VagAudio/Vpk|Pss|PswVideo -- DecoderRegistry only has Scene/Image/Text
    // slots today (Audio/Video/Schema "arrive with their first consumer",
    // per DecoderRegistry.h) -- left unregistered, matching GowrModule's
    // precedent for its own no-natural-fit types.
}

std::optional<Onyx::Modules::DecodedText> Gow2Module::DecodeTextPlain(DecodeContext& ctx) {
    const AssetEntryT& e = ctx.entry;
    if (e.source.size == 0) {
        ctx.diags.Report(Diag{Severity::Info, "gow2.text.empty",
                               "'" + e.name + "' has no payload", std::nullopt});
        return std::nullopt;
    }
    // FinalizeEntries already validated this entry's range during parse and
    // flags a node it found out-of-bounds as NodeFlags::Failed -- honor
    // that instead of re-deciding (and instead of blindly allocating for)
    // an entry already known to be bad.
    if (e.flags == NodeFlags::Failed) {
        ctx.diags.Report(Diag{Severity::Warning, "gow2.text.flagged-failed",
                               "'" + e.name + "' was flagged Failed during parse "
                               "(out-of-bounds range) -- not decoded", std::nullopt});
        return std::nullopt;
    }
    if (e.source.fileIndex >= ctx.doc.fileTable.size() ||
        !ctx.doc.fileTable[e.source.fileIndex]) {
        ctx.diags.Report(Diag{Severity::Error, "gow2.text.bad-file-index",
                               "'" + e.name + "' addresses file table slot " +
                                   std::to_string(e.source.fileIndex) + ", which is unavailable",
                               std::nullopt});
        return std::nullopt;
    }

    auto& file = ctx.doc.fileTable[e.source.fileIndex];
    const uint64_t fileSize = static_cast<uint64_t>(file->Size());
    if (e.source.offset >= fileSize) {
        ctx.diags.Report(Diag{Severity::Error, "gow2.text.offset-out-of-range",
                               "'" + e.name + "' offset " + std::to_string(e.source.offset) +
                                   " is past its file's size " + std::to_string(fileSize),
                               std::nullopt});
        return std::nullopt;
    }

    // Clamp to the file's remaining bytes rather than trusting
    // e.source.size outright: a corrupt tag stream could otherwise request
    // an allocation up to ~4 GiB (uint32_t size field) before any content
    // is even read.
    const uint64_t available   = fileSize - static_cast<uint64_t>(e.source.offset);
    const uint64_t clampedSize = std::min<uint64_t>(e.source.size, available);
    if (clampedSize == 0) {
        ctx.diags.Report(Diag{Severity::Error, "gow2.text.empty-after-clamp",
                               "'" + e.name + "' has no readable bytes within its file",
                               std::nullopt});
        return std::nullopt;
    }

    Vfs::SliceFile slice(file, static_cast<size_t>(e.source.offset),
                          static_cast<size_t>(clampedSize));
    std::string text(static_cast<size_t>(clampedSize), '\0');
    size_t got = slice.Read(text.data(), text.size());
    text.resize(got);

    Onyx::Modules::DecodedText out;
    out.text = std::move(text);

    std::string ext;
    size_t dot = e.name.find_last_of('.');
    if (dot != std::string::npos) {
        ext = e.name.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                        [](unsigned char c) { return char(std::tolower(c)); });
    }
    if (ext == "json")      out.language = "json";
    else if (ext == "csv")  out.language = "csv";
    else                    out.language = "";

    return out;
}

std::unique_ptr<Onyx::Parsers::SceneData> Gow2Module::DecodeSceneStub(DecodeContext& ctx) {
    ctx.diags.Report(Diag{
        Severity::Info, "gow2.scene.materials-deferred",
        "GOW2 model/mesh decoding needs per-part material resolution "
        "(GOW2MaterialParser + main-layer selection), which is Phase 3 "
        "scope -- '" + ctx.entry.name + "' was not decoded",
        std::nullopt});
    return nullptr;
}

std::unique_ptr<Onyx::Parsers::TextureData> Gow2Module::DecodeImage(DecodeContext& ctx) {
    const AssetEntryT& e = ctx.entry;

    if (e.flags == NodeFlags::Failed) {
        ctx.diags.Report(Diag{Severity::Warning, "gow2.texture.flagged-failed",
                               "'" + e.name + "' was flagged Failed during parse "
                               "(out-of-bounds range) -- not decoded", std::nullopt});
        return nullptr;
    }
    if (e.source.fileIndex >= ctx.doc.fileTable.size() ||
        !ctx.doc.fileTable[e.source.fileIndex]) {
        ctx.diags.Report(Diag{Severity::Error, "gow2.texture.bad-file-index",
                               "'" + e.name + "' addresses file table slot " +
                                   std::to_string(e.source.fileIndex) + ", which is unavailable",
                               std::nullopt});
        return nullptr;
    }

    // GOW2TextureParser::Parse needs the TXR entry's sibling GFX/PAL blocks
    // to decode pixels (not materials) -- mirrors the legacy call site
    // (TextureHandler.cpp:25, `GOW2TextureParser::Parse(entry, wad.entries,
    // wad.fileSource)`) exactly: ctx.doc.roots IS the same root-level
    // entry list `wad.entries` used to be (GOW2TextureParser's own
    // FindSibling walks that list plus one level of children, unchanged
    // here), and the bytes come from this entry's own fileIndex rather
    // than a single fixed fileSource, since GOW2 entries can now live in
    // any slot ParseIsoToc pushed into ctx.fileTable.
    auto tex = GOW2TextureParser::Parse(e, ctx.doc.roots, ctx.doc.fileTable[e.source.fileIndex]);
    if (!tex) {
        ctx.diags.Report(Diag{Severity::Warning, "gow2.texture.decode-failed",
                               "'" + e.name + "': GOW2TextureParser::Parse failed "
                               "(bad TXR magic, missing GFX/PAL sibling, or a GFX/PAL "
                               "parse failure -- see the log for which)",
                               std::nullopt});
        return nullptr;
    }
    return tex;
}

// ═══════════════════════════════════════════════════════════════════════════
// FinalizeEntries -- range salvage, shared by both ParseContainer bodies
// ═══════════════════════════════════════════════════════════════════════════

// Walks every produced node (recursing into folders) and flags -- but does
// NOT drop -- any entry whose payload range runs past the end of the file
// its (already-stamped) source.fileIndex names: Domain::NodeFlags::Failed is
// set and a diag names the entry and the offending range (salvage, spec
// sec7.1). Mirrors GowrModule.cpp's FinalizeEntries (Task 2) and
// OnyxBoxModule's reference pattern (Examples/OnyxBox/OnyxBoxModule.cpp,
// "obx.entry.range"). Unlike GowrModule's version, this one looks the file
// size up per-entry through `fileTable` rather than taking one fileSize for
// the whole tree -- GOW2's ISO path can have entries split across several
// PART*.PAK files, each a different fileTable slot with its own size.
static void FinalizeEntries(std::vector<AssetEntryT>& nodes,
                             const std::vector<std::shared_ptr<Vfs::IFile>>* fileTable,
                             DiagSink& diags) {
    for (auto& node : nodes) {
        if (!node.children.empty()) {
            FinalizeEntries(node.children, fileTable, diags);
        }
        if (node.source.size == 0) continue;  // folder/back-ref, no payload
        if (!fileTable || node.source.fileIndex >= fileTable->size() ||
            !(*fileTable)[node.source.fileIndex]) {
            continue;  // nothing to validate against — leave as-is
        }

        const uint64_t fileSize = (*fileTable)[node.source.fileIndex]->Size();
        const uint64_t end = static_cast<uint64_t>(node.source.offset) +
                              static_cast<uint64_t>(node.source.size);
        if (end > fileSize) {
            node.flags = NodeFlags::Failed;
            diags.Report(Diag{Severity::Error, "gow2.entry.range",
                "entry '" + node.name + "' payload out of bounds: offset=" +
                    std::to_string(node.source.offset) + " size=" +
                    std::to_string(node.source.size) + " exceeds file " +
                    std::to_string(node.source.fileIndex) + "'s size " +
                    std::to_string(fileSize),
                std::nullopt});
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Shared helper — ISO-path type assignment (ProfileGOW2's assignSchemaType)
// ═══════════════════════════════════════════════════════════════════════════

void Gow2Module::AssignSchemaType(AssetEntryT& entry) const {
    // Explicit default (Task 4 convergence): the pre-convergence code relied
    // on AssetEntryT::typeId's default-constructed value happening to equal
    // GameTypes::Unknown's legacy numeric value (0) -- a coincidence of the
    // old force-value registration, not a Types::TypeId{} guarantee under
    // this module's own TypeRegistrar-minted catalog. Set it explicitly so
    // "no extension" and "extension we don't recognise" both land on
    // m_unknown regardless of what value a default TypeId happens to carry.
    entry.typeId = m_unknown;

    size_t dot = entry.name.find_last_of('.');
    if (dot != std::string::npos) {
        std::string ext = entry.name.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

        if      (ext == "MDL") { entry.typeId = m_model;    }
        else if (ext == "TXR") { entry.typeId = m_texture;  }
        else if (ext == "ANM") { entry.typeId = m_animation; }
        else if (ext == "WAD") { entry.typeId = m_wadFile;  }
        else if (ext == "VAG") { entry.typeId = m_vagAudio; }
        else if (ext == "VPK" || ext == "VP1" || ext == "VP2" ||
                 ext == "VP3" || ext == "VP4")
                              { entry.typeId = m_vpkVideo; }
        else if (ext == "PSS") { entry.typeId = m_pssVideo; }
        else if (ext == "PSW") { entry.typeId = m_pswVideo; }
        else if (ext == "TXT" || ext == "INI" || ext == "CFG" ||
                 ext == "CSV" || ext == "JSON" || ext == "LOG")
                              { entry.typeId = m_textPlain; }
        // else: leave as m_unknown, set above.
    }
    entry.kind = Types::KindOf(entry.typeId);
}

// ═══════════════════════════════════════════════════════════════════════════
// ParseContainer — dispatch
// ═══════════════════════════════════════════════════════════════════════════

ParseResult Gow2Module::ParseContainer(ContainerContext& ctx) {
    // Mounts() declares only .iso, so ctx.mountedVfs is non-null exactly
    // when the opened path was an ISO that mounted successfully -- the same
    // split ProfileGOW2::Detect used to make by extension (.iso ->
    // MountArchive+LoadFromArchive, .wad -> ParseContainer's tag walk),
    // just driven by the SDK's own mount signal instead of re-checking the
    // extension here.
    if (ctx.mountedVfs) {
        return ParseIsoToc(ctx);
    }
    return ParseWadTagStream(ctx);
}

// ═══════════════════════════════════════════════════════════════════════════
// ParseWadTagStream — ProfileGOW2::ParseContainer, ported verbatim
// ═══════════════════════════════════════════════════════════════════════════

ParseResult Gow2Module::ParseWadTagStream(ContainerContext& ctx) {
    Vfs::IFile& file = ctx.file;
    if (!file.IsValid()) {
        ctx.diags.Report(Diag{Severity::Error, "gow2.file.invalid",
                               "container file is not valid", std::nullopt});
        return ParseResult{false};
    }

    file.Seek(0, SEEK_END);
    int64_t fileSize = file.Tell();
    file.Seek(0, SEEK_SET);

    ONYX_LOGF_INFO("[GOW2] Parsing WAD of size %lld bytes", (long long)fileSize);

    int64_t pos = 0;

    // Stack of pointers to vectors of entries. We start with the root vector.
    std::vector<std::vector<AssetEntryT>*> stack;
    stack.push_back(&ctx.roots);

    bool newGroupTag = false;
    int totalTags = 0;

    // Name → (typeId, offset, size) for resolving zero-sized reference entries.
    // A SERVER_INSTANCE with size=0 is a pointer to a previous definition with
    // the same name. When accessed, we redirect to the real definition's data
    // (mirrors reference GetNodeById lazy resolution). Kept as uint64_t (not
    // narrowed to uint32_t) -- the DefInfo narrowing bug the brief mentions
    // was already fixed upstream of this task, in Phase 1 commit 18f958b.
    struct DefInfo { Types::TypeId typeId; uint64_t offset; uint64_t size; };
    std::unordered_map<std::string, DefInfo> nameToDefinition;

    const std::string wadName = ctx.path.filename().string();

    while (pos < fileSize) {
        RawWadTag rawTag;
        if (file.Read(&rawTag, sizeof(RawWadTag)) != sizeof(RawWadTag)) {
            break; // EOF or error
        }

        pos += sizeof(RawWadTag);
        if (rawTag.tag == WADTAG_ENTITY_COUNT) rawTag.size = 0;

        // ── Handle structural tags that affect the stack but don't produce nodes ──
        if (rawTag.tag == WADTAG_GROUP_START) {
            newGroupTag = true;
            if (rawTag.size > 0) {
                pos += rawTag.size;
                pos = ((pos + 15) / 16) * 16;
                file.Seek(pos, SEEK_SET);
            }
            continue;
        }
        if (rawTag.tag == WADTAG_GROUP_END) {
            if (!newGroupTag) {
                if (stack.size() > 1) stack.pop_back();
            } else {
                newGroupTag = false; // empty group
            }
            if (rawTag.size > 0) {
                pos += rawTag.size;
                pos = ((pos + 15) / 16) * 16;
                file.Seek(pos, SEEK_SET);
            }
            continue;
        }
        // Entity count and header pop are metadata — skip silently (reference: default → NOP)
        if (rawTag.tag == WADTAG_ENTITY_COUNT || rawTag.tag == WADTAG_HEADER_POP) {
            if (rawTag.size > 0) {
                pos += rawTag.size;
                pos = ((pos + 15) / 16) * 16;
                file.Seek(pos, SEEK_SET);
            }
            continue;
        }
        // Only SERVER_INSTANCE (1), TT_* (11-16) and HEADER_START (21) reach the tree.
        // Any other unknown structural tag is silently ignored (reference: default → NOP).
        bool addToTree = (rawTag.tag == WADTAG_SERVER_INST) ||
                         (rawTag.tag >= 11 && rawTag.tag <= 16) ||
                         (rawTag.tag == WADTAG_HEADER_START);
        if (!addToTree) {
            if (rawTag.size > 0) {
                pos += rawTag.size;
                pos = ((pos + 15) / 16) * 16;
                file.Seek(pos, SEEK_SET);
            }
            continue;
        }

        AssetEntryT entry;
        entry.name   = std::string(rawTag.name, strnlen(rawTag.name, 24));
        entry.source.size   = rawTag.size;
        entry.source.offset = pos;
        entry.wadName = wadName;

        // ── Type Identification ──
        uint8_t payloadMagic[4] = {0};
        size_t payloadSizeAvailable = 0;
        if (rawTag.size >= 4) {
            file.Read(payloadMagic, 4);
            file.Seek(pos, SEEK_SET); // rewind
            payloadSizeAvailable = 4;
        }

        auto* handler = Gow::WadTypeRegistry::Get().ResolveByTag(Gow::GameVersion::GOW2, rawTag.tag, payloadMagic, payloadSizeAvailable);
        // handler->GetId(), when non-null, is still a legacy GameTypes::*
        // id sourced from the separate Source/core/types/handlers/*.cpp
        // self-registration subsystem -- a known, out-of-scope residual gap
        // documented on Gow2Module.h's member list, not exercised by
        // gowtoolkit_tests (those handler .cpp files aren't part of
        // APP_TEST_SOURCES, so ResolveByTag always returns nullptr here).
        entry.typeId = handler ? handler->GetId() : m_unknown;

        // Fallback type resolution for types not yet in Types::TypeRegistry
        if (entry.typeId == m_unknown) {
            size_t dotPos = entry.name.find_last_of('.');
            if (dotPos != std::string::npos) {
                std::string ext = entry.name.substr(dotPos + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

                if      (ext == "WAD") { entry.typeId = m_wadFile; }
                else if (ext == "VAG") { entry.typeId = m_vagAudio; }
                else if (ext == "VPK" || ext == "VP1") { entry.typeId = m_vpkVideo; }
                else if (ext == "PSS") { entry.typeId = m_pssVideo; }
                else if (ext == "PSW") { entry.typeId = m_pswVideo; }
                else if (ext == "TXT" || ext == "INI" || ext == "CFG" ||
                         ext == "CSV" || ext == "JSON" || ext == "LOG") {
                    entry.typeId = m_textPlain;
                }
            } else {
                std::string nameLower = entry.name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                if (nameLower.find("pal_") == 0) {
                    entry.typeId = m_palData;
                } else {
                    if (rawTag.size >= 4) {
                        uint32_t magic;
                        std::memcpy(&magic, payloadMagic, 4);
                        if ((magic & 0x80000000) != 0) {
                            entry.typeId = m_chunk;
                        } else {
                            ONYX_LOGF_INFO("[Gow2Module] Unknown tag: '%s' size=%u magic=0x%08X", entry.name.c_str(), rawTag.size, magic);
                        }
                    } else {
                        ONYX_LOGF_INFO("[Gow2Module] Unknown tag: '%s' size=%u (no magic)", entry.name.c_str(), rawTag.size);
                    }
                }
            }
        }

        // ── Zero-sized reference resolution (pass 1: same-name-seen-so-far) ──
        // A SERVER_INSTANCE with size=0 is a pointer to a previous definition
        // with the same name. Resolve it to the real data so decoders can
        // read it (mirrors reference GetNodeById lazy resolution).
        if (rawTag.tag == WADTAG_SERVER_INST && rawTag.size == 0) {
            auto it = nameToDefinition.find(entry.name);
            if (it != nameToDefinition.end()) {
                entry.typeId  = it->second.typeId;
                entry.source.offset  = it->second.offset;
                entry.source.size    = it->second.size;
            }
        } else if (rawTag.size > 0 && !entry.name.empty()) {
            // Cache real definitions for reference resolution above and pass 2
            nameToDefinition[entry.name] = { entry.typeId, entry.source.offset, entry.source.size };
        }

        entry.kind = Types::KindOf(entry.typeId);

        // ── Add node to tree ──
        std::vector<AssetEntryT>* currentLevel = stack.back();
        currentLevel->push_back(std::move(entry));
        totalTags++;

        // Only SERVER_INSTANCE (tag=1) can become the new parent after a GroupStart.
        // HeaderStart (tag=21) and TT_* (tags 11-16) are added as flat siblings without
        // group semantics — matches reference gow2parseTag exactly.
        if (newGroupTag && rawTag.tag == WADTAG_SERVER_INST) {
            newGroupTag = false;
            stack.push_back(&(currentLevel->back().children));
        }

        // Skip payload data (aligned to 16 bytes)
        if (rawTag.size > 0) {
            pos += rawTag.size;
            pos = ((pos + 15) / 16) * 16;
            file.Seek(pos, SEEK_SET);
        }
    }

    // ── Pass 2: resolve zero-sized/unknown entries from forward references ──
    // Load-bearing and subtle (task-3-brief.md Decision 3): pass 1 above only
    // catches a SERVER_INSTANCE reference whose definition was already seen
    // earlier in the stream. A reference that appears BEFORE its definition
    // is caught here instead, once the whole tree (and nameToDefinition) is
    // complete. Recurses into children so nested groups are covered too.
    std::function<void(std::vector<AssetEntryT>&)> resolveUnknowns = [&](std::vector<AssetEntryT>& list) {
        for (auto& n : list) {
            if (n.source.size == 0 && n.typeId == m_unknown && !n.name.empty()) {
                auto it = nameToDefinition.find(n.name);
                if (it != nameToDefinition.end()) {
                    n.typeId = it->second.typeId;
                    n.source.offset = it->second.offset;
                    n.source.size   = it->second.size;
                    n.kind = Types::KindOf(n.typeId);
                }
            }
            resolveUnknowns(n.children);
        }
    };
    resolveUnknowns(ctx.roots);

    // Every entry here lives in ctx.file itself, i.e. fileTable slot 0 (the
    // Workspace pre-seeds it with the container file, and ByteRange's
    // default fileIndex is already 0 -- nothing was pushed, so nothing
    // needs restamping; see FinalizeEntries' doc comment). Still run the
    // salvage pass: a corrupt/truncated tag stream can produce an entry
    // whose offset+size runs past fileSize even though the walker's own
    // pos<fileSize loop bound stayed satisfied for the tag header itself.
    FinalizeEntries(ctx.roots, ctx.fileTable, ctx.diags);

    ONYX_LOGF_INFO("[GOW2] Parsed WAD: %d elements built into a tree structure.", totalTags);
    return ParseResult{true};
}

// ═══════════════════════════════════════════════════════════════════════════
// ParseIsoToc — ProfileGOW2::LoadFromArchive + LoadFromArchiveGOW2, ported
// onto ctx.mountedVfs / ctx.fileTable
// ═══════════════════════════════════════════════════════════════════════════

ParseResult Gow2Module::ParseIsoToc(ContainerContext& ctx) {
    Vfs::IVirtualFileSystem* vfs = ctx.mountedVfs;

    // Try GOW2.TOC first (some builds use this name), fall back to
    // GODOFWAR.TOC (default GOW2 layout) — unchanged from LoadFromArchive.
    auto tocFile = vfs->OpenFile("/GOW2.TOC");
    if (!tocFile || !tocFile->IsValid()) {
        tocFile = vfs->OpenFile("/GODOFWAR.TOC");
    }
    if (!tocFile || !tocFile->IsValid()) {
        ctx.diags.Report(Diag{Severity::Error, "gow2.iso.no-toc",
                               "no TOC file found in ISO (tried GOW2.TOC, GODOFWAR.TOC)",
                               std::nullopt});
        return ParseResult{false};
    }

    // Sanity-check GOW2 header: first 4 bytes = file count (small integer),
    // and count * sizeof(RawTocEntryGOW2) + 4 must fit within the file.
    tocFile->Seek(0, SEEK_END);
    int64_t tocSize = tocFile->Tell();
    tocFile->Seek(0, SEEK_SET);

    uint32_t possibleCount = 0;
    tocFile->Read(&possibleCount, 4);
    tocFile->Seek(0, SEEK_SET);

    bool isGOW2 = (possibleCount > 0 && possibleCount < 200000) &&
                  ((int64_t)(possibleCount * sizeof(RawTocEntryGOW2) + 4) <= tocSize);
    if (!isGOW2) {
        ctx.diags.Report(Diag{Severity::Error, "gow2.iso.bad-toc",
            "TOC header does not look like GOW2 (count=" + std::to_string(possibleCount) +
                ", size=" + std::to_string(tocSize) +
                "); GOW1 ISOs are not supported by this module",
            std::nullopt});
        return ParseResult{false};
    }

    ONYX_LOGF_INFO("[GOW2] Detected GOW2 TOC format (%u entries).", possibleCount);
    ONYX_LOGF_INFO("[GOW2] Parsing TOC... size: %lld bytes.", (long long)tocSize);

    uint32_t numFiles = 0;
    if (tocFile->Read(&numFiles, 4) != 4) {
        ctx.diags.Report(Diag{Severity::Error, "gow2.iso.toc-truncated",
                               "failed to read TOC file count", std::nullopt});
        return ParseResult{false};
    }

    std::vector<RawTocEntryGOW2> rawEntries(numFiles);
    tocFile->Read(rawEntries.data(), numFiles * sizeof(RawTocEntryGOW2));

    const uint32_t offsetsStart = 4 + numFiles * (uint32_t)sizeof(RawTocEntryGOW2);

    const uint32_t SECTOR_SIZE     = 2048;
    const uint32_t DVDDL_SPLITLINE = 10000000;

    // pak name -> its slot in ctx.fileTable. Opened (and pushed) at most
    // once per pak, regardless of how many TOC entries reference it — this
    // is the first live multi-file addressing in the app (task-3-brief.md
    // Decision 2): every entry stamps source.fileIndex from what push_back
    // returned for ITS pak, never a guessed constant, so entries split
    // across PART1.PAK/PART2.PAK/... resolve against the right bytes.
    std::unordered_map<std::string, uint32_t> pakFileIndex;
    // Paks already confirmed missing/unopenable. Checked BEFORE calling
    // OpenFile again, not just before re-reporting the diag: a missing pak
    // (e.g. the second layer of a dual-layer disc that wasn't merged in) is
    // typically referenced by many TOC rows, and re-opening a file that is
    // already known not to exist for every one of them is wasted I/O on top
    // of the wasted diag the old (deduped-diag-only) code already avoided.
    std::set<std::string> missingPaks;

    for (const auto& raw : rawEntries) {
        if (raw.encountersCount == 0) continue;

        // offsetsStart + encountersStart*4 is a computed file position that
        // has never been validated against the TOC's actual size -- a
        // corrupt/truncated TOC can point this past EOF. Reading past EOF
        // wouldn't crash (Seek/Read just fail), but readSector would then
        // stay zero-initialized and silently produce a fabricated entry at
        // PART1.PAK sector 0, indistinguishable from a real one. Diag and
        // skip constructing an entry for this row instead of inventing one.
        const uint64_t offsetPos =
            static_cast<uint64_t>(offsetsStart) +
            static_cast<uint64_t>(raw.encountersStart) * 4;
        if (offsetPos + 4 > static_cast<uint64_t>(tocSize)) {
            ctx.diags.Report(Diag{Severity::Error, "gow2.iso.offset-out-of-range",
                "TOC entry '" + std::string(raw.name, strnlen(raw.name, 24)) +
                    "' encountersStart offset " + std::to_string(offsetPos) +
                    " is past the TOC's size " + std::to_string(tocSize) +
                    " -- skipped rather than resolving a fabricated sector 0",
                std::nullopt});
            continue;
        }

        uint32_t rawSector = 0;
        tocFile->Seek(static_cast<int64_t>(offsetPos), SEEK_SET);
        tocFile->Read(&rawSector, 4);

        uint32_t pakIndex   = rawSector / DVDDL_SPLITLINE;
        uint32_t realSector = rawSector % DVDDL_SPLITLINE;
        std::string pakName = "PART" + std::to_string(pakIndex + 1) + ".PAK";

        uint32_t fileIndex = 0;
        auto cached = pakFileIndex.find(pakName);
        if (cached != pakFileIndex.end()) {
            fileIndex = cached->second;
        } else if (missingPaks.count(pakName)) {
            continue;  // already confirmed missing -- diag already reported once
        } else {
            auto pakFile = vfs->OpenFile("/" + pakName);
            if (!pakFile || !pakFile->IsValid()) {
                pakFile = vfs->OpenFile(pakName);
            }
            if (!pakFile || !pakFile->IsValid()) {
                missingPaks.insert(pakName);
                ctx.diags.Report(Diag{Severity::Warning, "gow2.iso.pak-missing",
                    "'" + pakName + "' not found in ISO — skipping its entries "
                    "(dual-layer ISOs may need both layers merged)",
                    std::nullopt});
                continue;
            }
            if (!ctx.fileTable) {
                ctx.diags.Report(Diag{Severity::Error, "gow2.filetable.unavailable",
                    "'" + pakName + "' opened but there is no file table slot to "
                    "address it from; its entries cannot be addressed",
                    std::nullopt});
                continue;
            }
            std::shared_ptr<Vfs::IFile> shared(std::move(pakFile));
            fileIndex = static_cast<uint32_t>(ctx.fileTable->size());
            ctx.fileTable->push_back(shared);
            pakFileIndex[pakName] = fileIndex;
        }

        AssetEntryT entry;
        entry.name   = std::string(raw.name, strnlen(raw.name, 24));
        entry.source.size      = raw.size;
        entry.source.offset    = (int64_t)realSector * SECTOR_SIZE;
        entry.source.fileIndex = fileIndex;
        entry.wadName = pakName;
        entry.hash = std::hash<std::string>{}(entry.name);
        AssignSchemaType(entry);
        ctx.roots.push_back(std::move(entry));
    }

    FinalizeEntries(ctx.roots, ctx.fileTable, ctx.diags);

    ONYX_LOGF_INFO("[GOW2] TOC parsed: %zu files.", ctx.roots.size());
    return ParseResult{!ctx.roots.empty()};
}

} // namespace Onyx::Gow2
