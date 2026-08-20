#include "GowrModule.h"

#include <Onyx/Modules/Workspace.h>
#include <Onyx/Vfs/MemoryFile.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Fonts/SFSymbols.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

#include <lz4frame.h>

#include "core/WadTypes.h"
#include "core/profiles/gowr/GOWRTypes.h"
#include "core/profiles/gowr/WadNodeBuilder.h"
#include "core/loaders/GOWRLoaders.h"
#include "core/parsers/gowr/TextureDecode.h"

// ── GowrModule.cpp ─────────────────────────────────────────────────────────
// Ported from ProfileGOWR.{h,cpp} (core/profiles/gowr/). ParseContainer's
// body is the same algorithm — LZ4-sniff, decompress if needed, read the
// WTOC header + FileDesc table, resolve absolute offsets via the
// blockBitSet/flush-queue walk, hand the flat array to WadNodeBuilder — with
// these changes from the original:
//
//   - outWad.entries -> ctx.roots (WadNodeBuilder is unchanged; a scratch
//     AssetContainer receives its output and ctx.roots takes the entries by
//     move, since ContainerContext has no AssetContainer of its own).
//   - The decompressed buffer -> GowrModuleState::decompressedFile, held by
//     ctx.state, instead of AssetContainer::fileSource (see GowrModule.h).
//   - Failures report through ctx.diags (Severity + a namespaced code), not
//     only ONYX_LOGF_ERR.
//   - PrepareForParse's job (EnsureGowrConfigIni + InvalidateLodIndex) now
//     runs at the top of ParseContainer itself, using ctx.path (the
//     container path ContainerContext gained in the 74f0c73 SDK repin) —
//     see the comment at the top of ParseContainer.
//   - AssetEntry::wadName is filled from ctx.path.filename().string(),
//     passed to WadNodeBuilder::Build as wadFilename — see the comment
//     above that call.

namespace Onyx::Gowr {

using Onyx::Domain::MediaKind;
using Onyx::Modules::ContainerContext;
using Onyx::Modules::DecodeContext;
using Onyx::Modules::DecoderRegistry;
using Onyx::Modules::ModuleInfo;
using Onyx::Modules::OpenFilter;
using Onyx::Modules::ParseResult;
using Onyx::Modules::ProbeInput;
using Onyx::Modules::ProbeResult;
using Onyx::Services::Diag;
using Onyx::Services::Severity;

// ═══════════════════════════════════════════════════════════════════════════
// Info / Probe
// ═══════════════════════════════════════════════════════════════════════════

ModuleInfo GowrModule::Info() const {
    return ModuleInfo{
        "gowr",
        "God of War Ragnarok (PS4/PS5)",
        {"gowr", "ragnarok", "ps4", "ps5"},
        {OpenFilter{"God of War (2018) / Ragnarok", {"wad"}}},
    };
}

// Evidence-scored per spec (docs/superpowers/specs/2026-08-20-port-phase-2-
// game-modules-design.md, Decision 1): 95 for a real WTOC magic or LZ4 frame
// header actually seen in the supplied bytes, 10 for a bare .wad with
// neither. The old ProfileGOWR::Detect's "unless it sniffs GOW2" exclusion is
// gone -- under RankProbes (>= 40 AND strictly beats the runner-up), GOWR
// simply outscores GOW2's 45-for-a-bare-.wad on real evidence; nothing needs
// to know about the other module. Pure: only path/header/fileSize, no I/O.
ProbeResult GowrModule::Probe(const ProbeInput& in) const {
    if (in.header.size() >= 4) {
        uint32_t magic = 0;
        std::memcpy(&magic, in.header.data(), 4);
        if (magic == 0x434F5457) {
            return ProbeResult{95, "WTOC magic at 0"};
        }
        if (magic == 0x184D2204) {
            return ProbeResult{95, "LZ4 frame header"};
        }
    }

    std::string ext = in.path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return char(std::tolower(c)); });
    if (ext == ".wad") {
        return ProbeResult{10, "bare .wad extension, no WTOC/LZ4 evidence"};
    }

    return ProbeResult{0, "no evidence"};
}

// ═══════════════════════════════════════════════════════════════════════════
// RegisterTypes
// ═══════════════════════════════════════════════════════════════════════════

// Mints one module-scoped TypeId per type WadNodeBuilder::RoleToTypeId can
// actually produce for a GOWR entry (core/profiles/gowr/WadNodeBuilder.cpp),
// keyed/labelled/coloured to match the corresponding row of
// core/types/GameTypeTable.h (rows 27-50: the Shader* family through
// GowrMaterial -- rows 0-26 are either structural placeholders shared with
// GOW2's legacy tag stream or, per grep across core/profiles/gow2, unused by
// GOWR). Keys are bare per TypeRegistrar::Add's contract; the registrar
// namespaces them under "gowr." itself.
//
// See GowrModule.h's member comment: these handles do not (yet) appear on
// any AssetEntry the tree actually contains -- WadNodeBuilder still stamps
// the legacy Onyx::GameTypes::* externs. Reported as a gap in
// task-2-report.md.
void GowrModule::RegisterTypes(Onyx::Types::TypeRegistrar& r) {
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

    m_shaderContainer    = add("shaderContainer",    "Shader",           MediaKind::Shader,    ICON_SF_CURLYBRACES, 0.5f, 1.0f, 0.5f, 1.0f);
    m_shaderVertex       = add("shaderVertex",       "Vertex Shader",    MediaKind::Shader,    ICON_SF_CURLYBRACES, 0.5f, 1.0f, 0.5f, 1.0f);
    m_shaderPixel        = add("shaderPixel",        "Pixel Shader",     MediaKind::Shader,    ICON_SF_CURLYBRACES, 0.5f, 1.0f, 0.5f, 1.0f);
    m_shaderHull         = add("shaderHull",         "Hull Shader",      MediaKind::Shader,    ICON_SF_CURLYBRACES, 0.5f, 1.0f, 0.5f, 1.0f);
    m_shaderDomain       = add("shaderDomain",       "Domain Shader",    MediaKind::Shader,    ICON_SF_CURLYBRACES, 0.5f, 1.0f, 0.5f, 1.0f);
    m_shaderCompute      = add("shaderCompute",      "Compute Shader",   MediaKind::Shader,    ICON_SF_CURLYBRACES, 0.5f, 1.0f, 0.5f, 1.0f);
    m_shaderLibrary      = add("shaderLibrary",      "Library Shader",   MediaKind::Shader,    ICON_SF_CURLYBRACES, 0.5f, 1.0f, 0.5f, 1.0f);
    m_meshGpu            = add("meshGpu",            "Mesh GPU",         MediaKind::Mesh,      ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_meshDefn           = add("meshDefn",           "Mesh Definition",  MediaKind::Mesh,      ICON_SF_CUBE_FILL,   0.4f, 0.8f, 1.0f, 1.0f);
    m_gameObjectProto    = add("gameObjectProto",    "GO Proto",         MediaKind::Skeleton,  ICON_SF_PERSON_FILL, 1.0f, 0.6f, 0.3f, 1.0f);
    m_gameObjectInst     = add("gameObjectInst",     "GO Instance",      MediaKind::Skeleton,  ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_gameObjectOverride = add("gameObjectOverride", "GO Override",      MediaKind::Skeleton,  ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_texturePair        = add("texturePair",        "Texture Pair",     MediaKind::Image,     ICON_SF_PHOTO,       1.0f, 0.5f, 0.8f, 1.0f);
    m_materialRef        = add("materialRef",        "Material Ref",     MediaKind::Material,  ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_lodBinding         = add("lodBinding",         "LOD Binding",      MediaKind::Mesh,      ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_animClip           = add("animClip",           "Anim Clip",        MediaKind::Animation, ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_soundEmitter       = add("soundEmitter",       "Sound Emitter",    MediaKind::Audio,     ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_particleEmitter    = add("particleEmitter",    "Particle Emitter", MediaKind::Raw,       ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_particleSystem     = add("particleSystem",     "Particle System",  MediaKind::Raw,       ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_clientGuid         = add("clientGuid",         "Client GUID",      MediaKind::Raw,       ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_wadIdentity        = add("wadIdentity",        "WAD Identity",     MediaKind::Container, ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_sharedWadRef       = add("sharedWadRef",       "Shared WAD Ref",   MediaKind::Container, ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_sentinel           = add("sentinel",           "Sentinel",         MediaKind::Unknown,   ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
    m_material           = add("material",           "Material",         MediaKind::Material,  ICON_SF_DOCUMENT,    0.6f, 0.6f, 0.6f, 1.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// RegisterDecoders
// ═══════════════════════════════════════════════════════════════════════════

void GowrModule::RegisterDecoders(DecoderRegistry& reg) {
    reg.Image(m_texturePair, &GowrModule::DecodeTexturePair);

    // Materials are Phase 3 -- see GowrModule.h and DecodeMeshStub. Wired
    // (rather than left unregistered) so a caller gets an explicit diag
    // instead of DecoderRegistry silently reporting "no decoder".
    reg.Scene(m_meshDefn,           &GowrModule::DecodeMeshStub);
    reg.Scene(m_meshGpu,            &GowrModule::DecodeMeshStub);
    reg.Scene(m_gameObjectProto,    &GowrModule::DecodeMeshStub);
    reg.Scene(m_gameObjectInst,     &GowrModule::DecodeMeshStub);
    reg.Scene(m_gameObjectOverride, &GowrModule::DecodeMeshStub);

    // No TextDecoder: no GOWR type WadNodeBuilder produces maps to a
    // text/script MediaKind today (shaders are binary, not source, in this
    // WAD format). DecoderRegistry.h documents Text/Schema slots arriving
    // with their first consumer -- GOWR isn't one yet.
}

std::unique_ptr<Onyx::Parsers::TextureData> GowrModule::DecodeTexturePair(DecodeContext& ctx) {
    // The texture's content hash is the hex suffix trailing the entry's own
    // name (e.g. "TX_angrboda_fox00_head_gen_0d_1D293ECA4DE04637" ->
    // 0x1D293ECA4DE04637) -- ported from GOWRTextureViewer's constructor
    // (core/loaders/GOWRLoaders.cpp). The pixel data lives in an external
    // texpack the hash resolves through, never in this entry's own
    // ByteRange, so this decoder needs neither ctx.doc.file nor
    // GowrModuleState.
    const std::string& name = ctx.entry.name;
    auto lastUs = name.find_last_of('_');
    if (lastUs == std::string::npos || lastUs + 1 >= name.size()) {
        ctx.diags.Report(Diag{Severity::Error, "gowr.texture.no-hash",
                               "texture entry name carries no trailing hex hash: " + name,
                               std::nullopt});
        return nullptr;
    }

    uint64_t hash = 0;
    try {
        hash = std::stoull(name.substr(lastUs + 1), nullptr, 16);
    } catch (...) {
        ctx.diags.Report(Diag{Severity::Error, "gowr.texture.bad-hash",
                               "texture entry name's trailing suffix is not a hex hash: " + name,
                               std::nullopt});
        return nullptr;
    }

    // GetTexIndex() is a process-wide singleton (core/loaders/GOWRLoaders.cpp)
    // that lazily kicks off a background indexing pass the first time it's
    // called -- see this file's ParseContainer for the detached-thread note.
    TexPackIndex& index = GetTexIndex();

    auto tex = std::make_unique<Onyx::Parsers::TextureData>();
    std::string error;
    if (!GOWRDecodeTexture(index, hash, name, *tex, error)) {
        ctx.diags.Report(Diag{Severity::Warning, "gowr.texture.decode-failed",
                               name + ": " + error, std::nullopt});
        return nullptr;
    }
    return tex;
}

std::unique_ptr<Onyx::Parsers::SceneData> GowrModule::DecodeMeshStub(DecodeContext& ctx) {
    ctx.diags.Report(Diag{
        Severity::Info, "gowr.mesh.materials-deferred",
        "GOWR mesh/rig decoding needs per-part material resolution "
        "(TextureRole/MaterialDesc), which is Phase 3 scope -- '" +
            ctx.entry.name + "' was not decoded",
        std::nullopt});
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// ParseContainer
// ═══════════════════════════════════════════════════════════════════════════

Onyx::Modules::ParseResult GowrModule::ParseContainer(ContainerContext& ctx) {
    // ── PrepareForParse's job (ProfileGOWR::PrepareForParse) ──────────────
    // The original profile interface called this BEFORE ParseContainer, and
    // its ordering was load-bearing: EnsureGowrConfigIni(wadPath) writes
    // config.ini (auto-detecting the game root) the first time a WAD opens,
    // and InvalidateLodIndex() drops any LOD/texpack index singleton that
    // may have been built (and cached) without one -- AssetHarness.h's
    // header comment documents this as a real, previously-shipped bug (the
    // old CLI, which skipped PrepareForParse, could parse a different mesh
    // set than the GUI from the same file). IGameModule has no pre-parse
    // hook, so this now runs at the very top of ParseContainer instead,
    // before anything touches the LOD index.
    //
    // ContainerContext now carries the container's path (SDK repin to
    // 74f0c73 / Onyx commit 6a9a980 here): `ctx.path`
    // (Include/Onyx/Modules/Workspace.h) -- for a mounted container this
    // still names the OUTER container (the file the user opened), which is
    // exactly what PrepareForParse walked from originally, so this is a
    // faithful, not approximate, port.
    if (EnsureGowrConfigIni(ctx.path)) {
        InvalidateLodIndex();
    }

    Vfs::IFile& file = ctx.file;
    if (!file.IsValid()) {
        ctx.diags.Report(Diag{Severity::Error, "gowr.file.invalid",
                               "container file is not valid", std::nullopt});
        return ParseResult{false};
    }

    file.Seek(0, SEEK_END);
    int64_t fileSize = file.Tell();
    file.Seek(0, SEEK_SET);

    uint32_t initialMagic = 0;
    file.Read(&initialMagic, 4);

    // Non-null only once LZ4 decompression actually runs below -- see
    // GowrModuleState's comment in GowrModule.h for why a plain WTOC
    // container needs no override here at all.
    std::shared_ptr<Vfs::IFile> decompressed;

    if (initialMagic == 0x184D2204) {
        ONYX_LOGF_INFO("[GOWR] WAD is LZ4-compressed. Decompressing...");

        size_t dstCapacity = 0;
        file.Seek(0x06, SEEK_SET);
        file.Read(&dstCapacity, 4);

        if (dstCapacity == 0 || dstCapacity > 1024ULL * 1024 * 500) {
            ctx.diags.Report(Diag{Severity::Error, "gowr.lz4.bad-size",
                                   "invalid decompressed size: " + std::to_string(dstCapacity),
                                   std::nullopt});
            return ParseResult{false};
        }

        std::vector<uint8_t> src(static_cast<size_t>(fileSize));
        file.Seek(0, SEEK_SET);
        file.Read(src.data(), static_cast<size_t>(fileSize));

        std::vector<uint8_t> dst(dstCapacity);

        LZ4F_dctx* lz4ctx = nullptr;
        LZ4F_createDecompressionContext(&lz4ctx, LZ4F_getVersion());
        LZ4F_decompressOptions_t opn = { 0, 0, 0, 0 };

        size_t srcSize = static_cast<size_t>(fileSize);
        size_t outSize = dstCapacity;
        size_t ret = LZ4F_decompress(lz4ctx, dst.data(), &outSize, src.data(), &srcSize, &opn);
        LZ4F_freeDecompressionContext(lz4ctx);

        // ProfileGOWR::ParseContainer never checked this return value either
        // (a latent gap in the ported code, not introduced here) -- reported
        // now rather than silently ignored, but non-fatal to match the
        // original control flow exactly (the golden fixture decompresses
        // cleanly).
        if (LZ4F_isError(ret)) {
            ctx.diags.Report(Diag{Severity::Warning, "gowr.lz4.decode-error",
                                   std::string("LZ4F_decompress reported: ") +
                                       LZ4F_getErrorName(ret),
                                   std::nullopt});
        }

        decompressed = std::make_shared<Vfs::MemoryFile>(std::move(dst));
        fileSize     = static_cast<int64_t>(outSize);
        ONYX_LOGF_INFO("[GOWR] Decompressed to %lld bytes.", static_cast<long long>(fileSize));
    }

    Vfs::IFile& parsed = decompressed ? *decompressed : file;
    parsed.Seek(0, SEEK_SET);

    // ── Read WTOC header ───────────────────────────────────────────────────
    GOWRWadHeader header;
    if (parsed.Read(&header, sizeof(GOWRWadHeader)) != sizeof(GOWRWadHeader)) {
        ctx.diags.Report(Diag{Severity::Error, "gowr.header.truncated",
                               "failed to read WAD header", std::nullopt});
        return ParseResult{false};
    }

    if (header.magic != 0x434F5457) {
        char magicHex[9] = {};
        std::snprintf(magicHex, sizeof(magicHex), "%08X", header.magic);
        ctx.diags.Report(Diag{Severity::Error, "gowr.header.bad-magic",
                               std::string("invalid magic: 0x") + magicHex +
                                   " (expected WTOC = 0x434F5457)",
                               std::nullopt});
        return ParseResult{false};
    }
    if (header.ver != 0x2) {
        ctx.diags.Report(Diag{Severity::Error, "gowr.header.bad-version",
                               "unsupported version " + std::to_string(header.ver) +
                                   " (expected 2)",
                               std::nullopt});
        return ParseResult{false};
    }

    // A container's absence of a decompression pass still needs its state
    // recorded (harmless when null -- see GowrModuleState) so downstream
    // consumers have a single, consistent place to check regardless of
    // which branch below ran.
    auto state = std::make_shared<GowrModuleState>();
    state->decompressedFile = decompressed;
    ctx.state = state;

    if (header.fileCount == 0) {
        ONYX_LOGF_INFO("[GOWR] WAD contains 0 files.");
        return ParseResult{true};
    }

    ONYX_LOGF_INFO("[GOWR] WAD header: %u files, block0=%u, block1=%u, block2=%u",
                    header.fileCount, header.block0Size, header.block1Size, header.block2Size);

    // ── Read all FileDesc entries ──────────────────────────────────────────
    // GOWRFileDesc is fixed-size (0x90 bytes, not self-describing), so a
    // truncated table has no per-entry salvage point: every record after the
    // short read is equally unreadable. Matches ProfileGOWR's original
    // all-or-nothing behaviour rather than inventing partial recovery.
    std::vector<GOWRFileDesc> fileDescs(header.fileCount);
    for (uint32_t i = 0; i < header.fileCount; i++) {
        if (parsed.Read(&fileDescs[i], sizeof(GOWRFileDesc)) != sizeof(GOWRFileDesc)) {
            ctx.diags.Report(Diag{Severity::Error, "gowr.descriptor.truncated",
                                   "failed to read file descriptor " + std::to_string(i),
                                   std::nullopt});
            return ParseResult{false};
        }
    }

    // ── Compute absolute offsets via blockBitSet/flush-queue algorithm ────
    // Ported verbatim from ProfileGOWR::ParseContainer (itself ported from
    // GOWTool's Wad.cpp). Must run before WadNodeBuilder because it resolves
    // the correct in-file offset for each entry.
    std::vector<size_t> absOffsets(header.fileCount, 0);
    {
        size_t readOff = static_cast<size_t>(parsed.Tell());  // base = after header + all descs
        std::map<uint8_t, uint32_t>              bitsetOffs;
        std::map<uint8_t, std::vector<uint32_t>> flushQ;

        for (uint32_t i = 0; i < header.fileCount; i++) {
            std::string nameStr(fileDescs[i].name,
                strnlen(fileDescs[i].name, sizeof(fileDescs[i].name)));

            if (fileDescs[i].unk3[0x2] == 1) {
                if (nameStr != "autopad")
                    flushQ[fileDescs[i].blockBitSet].push_back(i);
                if (fileDescs[i].unk2[20] != 0)
                    flushQ[8].push_back(i);

                for (auto& [key, queue] : flushQ) {
                    readOff -= bitsetOffs[key];
                    uint32_t temp = bitsetOffs[key];
                    if (!queue.empty()) {
                        for (auto idx : queue) {
                            if (key == 8 && fileDescs[idx].blockBitSet != 8) {
                                temp = fileDescs[idx].offset2 + 16;
                                bitsetOffs[key] = temp;
                            } else {
                                absOffsets[idx]  = readOff + fileDescs[idx].offset;
                                bitsetOffs[key]  = fileDescs[idx].offset + fileDescs[idx].size;
                                temp             = bitsetOffs[key];
                            }
                        }
                        queue.clear();
                    }
                    readOff += temp;
                }
                if (nameStr == "autopad") {
                    absOffsets[i] = readOff;
                    readOff      += fileDescs[i].size;
                }
            } else {
                flushQ[fileDescs[i].blockBitSet].push_back(i);
                if (fileDescs[i].unk2[20] != 0)
                    flushQ[8].push_back(i);
            }
        }

        // Final flush
        for (auto& [key, queue] : flushQ) {
            if (!queue.empty()) {
                readOff -= bitsetOffs[key];
                uint32_t temp = 0;
                for (auto idx : queue) {
                    if (key == 8 && fileDescs[idx].blockBitSet != 8) {
                        temp = fileDescs[idx].offset2 + 16;
                        bitsetOffs[key] = temp;
                    } else {
                        absOffsets[idx] = readOff + fileDescs[idx].offset;
                        bitsetOffs[key] = fileDescs[idx].offset + fileDescs[idx].size;
                        temp            = bitsetOffs[key];
                    }
                }
                queue.clear();
                readOff += temp;
            }
        }
    }

    // ── Build semantic tree ────────────────────────────────────────────────
    // WadNodeBuilder is unchanged (core/profiles/gowr/WadNodeBuilder.h/.cpp);
    // it still takes an AssetContainer&, not ContainerContext::roots
    // directly, so a scratch container receives its output and ctx.roots
    // takes the entries by move. wadFilename is ctx.path's filename
    // component only (not the full path, not the stem) -- matching every
    // other constructor of this value in the tree: AssetHarness.cpp's
    // `out.container.filename = req.archive.filename().string()` and
    // golden_helpers.cpp's `wad.filename = wadPath.filename().string()`.
    // The only consumer of this argument is AssetEntry::wadName, a cosmetic
    // display-only field -- SnapshotEntries in tests/golden_helpers.cpp
    // reads name/typeId/size/offset/childCount/kind/payloadHash, never
    // wadName, and it appears in neither golden fixture, so this value is
    // not a pinned contract; matching prior behaviour is still correct.
    AssetContainer scratch;
    WadNodeBuilder builder;
    builder.Build(fileDescs, absOffsets, ctx.path.filename().string(), scratch);
    ctx.roots = std::move(scratch.entries);

    ONYX_LOGF_INFO("[GOWR] Parsed WAD: %u entries -> %zu root nodes.",
                    header.fileCount, ctx.roots.size());

    // ── Eager TexPack indexing ──────────────────────────────────────────────
    // Unchanged: GetTexIndex() (core/loaders/GOWRLoaders.cpp) itself spins up
    // a std::thread(...).detach() the first time it is called, process-wide.
    // ContainerContext exposes no Services::JobQueue (confirmed from its
    // field list in Include/Onyx/Modules/Workspace.h -- file, settings,
    // diags, progress, state, roots, mountedVfs, fileTable; no jobs
    // accessor), so this module has no way to move that work onto the
    // Workspace's job lanes. The detached thread is a real hazard (it can
    // outlive the Document that triggered it) and stays exactly that --
    // named here and in task-2-report.md, not silently carried over.
    GetTexIndex();

    return ParseResult{true};
}

} // namespace Onyx::Gowr
