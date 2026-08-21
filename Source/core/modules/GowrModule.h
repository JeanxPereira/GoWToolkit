#pragma once

// ── GowrModule ─────────────────────────────────────────────────────────────
// Onyx v1.1 `IGameModule` for God of War Ragnarok (PS4/PS5). Replaces
// `Onyx::ProfileGOWR` (core/profiles/gowr/ProfileGOWR.{h,cpp}), which
// implemented the now-deleted `Domain::IAssetProfile`.
//
// ParseContainer() reuses WadNodeBuilder (core/profiles/gowr/WadNodeBuilder.h)
// unchanged -- it is GOWR's tree assembler and stays put; only the frame
// around it (how the container is opened, where the decompressed buffer
// lives, how failures are reported) changes for the new interface. See
// GowrModule.cpp's ParseContainer for a port-by-port account of what
// differs from ProfileGOWR::ParseContainer, and task-2-report.md for the
// full list of decisions and gaps.

#include <Onyx/Modules/GameModule.h>
#include <Onyx/Modules/DecoderRegistry.h>
#include <Onyx/Vfs/IFile.h>

#include <memory>

namespace Onyx::Gowr {

// Per-document state ParseContainer stashes in ContainerContext::state (and
// therefore reachable from decoders via DecodeContext::doc.state, since
// Document::state is the same shared_ptr<void>).
//
// ProfileGOWR::ParseContainer LZ4-decompressed a compressed container into a
// MemoryFile and overwrote AssetContainer::fileSource with it, so every
// downstream SliceFile transparently read the decompressed buffer instead of
// the original (still-compressed) bytes. IGameModule has no equivalent
// per-entry "fileSource override" -- ContainerContext::roots' ByteRange
// entries are meant to be read through the Document's own file (or, for a
// mount, doc.fileTable[fileIndex]). This struct is the module-owned stand-in:
// a consumer that would have read `AssetContainer::fileSource` must now check
// `decompressedFile` first and fall back to the document's own file only when
// it is null.
struct GowrModuleState {
    // Null when the container was never LZ4-compressed to begin with (WTOC
    // magic at offset 0 -- no LZ4 frame, nothing to decompress, downstream
    // readers use the document's own file directly). Non-null only after
    // ParseContainer decompressed a LZ4-framed container in memory; every
    // ByteRange::offset the tree carries is relative to *this* buffer, not
    // the original (compressed) container file.
    std::shared_ptr<Onyx::Vfs::IFile> decompressedFile;
};

class GowrModule final : public Onyx::Modules::IGameModule {
public:
    Onyx::Modules::ModuleInfo  Info() const override;
    Onyx::Modules::ProbeResult Probe(const Onyx::Modules::ProbeInput&) const override;
    void RegisterTypes(Onyx::Types::TypeRegistrar&) override;
    void RegisterDecoders(Onyx::Modules::DecoderRegistry&) override;
    Onyx::Modules::ParseResult ParseContainer(Onyx::Modules::ContainerContext&) override;

private:
    // ── Decoders (bound in RegisterDecoders) ──────────────────────────────

    // TexturePair -> RGBA8. Self-contained: the pixel data lives in an
    // external texpack resolved by the hex hash trailing the entry's own
    // name (GetTexIndex()/GOWRDecodeTexture, core/parsers/gowr/TextureDecode.h),
    // never in the WAD entry's own byte range, so this needs no material
    // model at all.
    static std::unique_ptr<Onyx::Parsers::TextureData>
        DecodeTexturePair(Onyx::Modules::DecodeContext&);

    // Mesh/rig/instance decoding (MeshDefn, MeshGpu, GameObjectProto,
    // GameObjectInst, GameObjectOverride) is NOT ported. Every one of these
    // resolves per-part materials before it can build a Parsers::SceneData
    // (SharedGowrMeshLoad in core/loaders/GOWRLoaders.cpp calls
    // GOWRMaterialParse and indexes MaterialDesc::textures by TextureRole),
    // and TextureRole/MaterialDesc are explicitly Phase 3 scope. Stubbed per
    // the task brief: reports a diag and returns null rather than a partial
    // port. See task-2-report.md.
    static std::unique_ptr<Onyx::Parsers::SceneData>
        DecodeMeshStub(Onyx::Modules::DecodeContext&);

    // ── Types minted in RegisterTypes(), consumed by RegisterDecoders() ───
    // Phase 2 Task 4 closed the disconnect task-2-report.md flagged:
    // WadNodeBuilder no longer hardcodes Onyx::GameTypes::* -- ParseContainer
    // hands it a RoleToTypeIdFn (WadNodeBuilder.h) built from these handles,
    // so a real parsed tree's AssetEntry::typeId values ARE these
    // module-scoped ids, and RegisterDecoders()'s wiring (below) now fires
    // against entries this module's own ParseContainer produces. m_unknown
    // is new in Task 4 -- RegisterTypes() previously minted no "unknown"
    // key even though WadEntryRole::Unknown (and every synthetic-folder
    // role Classify() can't reconstruct) needs one; see task-4-report.md.
    Onyx::Types::TypeId m_shaderContainer, m_shaderVertex, m_shaderPixel, m_shaderHull,
        m_shaderDomain, m_shaderCompute, m_shaderLibrary, m_meshGpu, m_meshDefn,
        m_gameObjectProto, m_gameObjectInst, m_gameObjectOverride, m_texturePair,
        m_materialRef, m_lodBinding, m_animClip, m_soundEmitter, m_particleEmitter,
        m_particleSystem, m_clientGuid, m_wadIdentity, m_sharedWadRef, m_sentinel,
        m_material, m_unknown;
};

} // namespace Onyx::Gowr
