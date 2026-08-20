#pragma once

// ── Gow2Module ─────────────────────────────────────────────────────────────
// Onyx v1.1 `IGameModule` for God of War I/II (PS2). Replaces
// `Onyx::ProfileGOW2` (core/profiles/gow2/ProfileGOW2.{h,cpp}), which
// implemented the now-deleted `Domain::IAssetProfile`.
//
// Two independent parse paths, matching the two things ProfileGOW2 used to
// open:
//
//   - A bare .wad: ParseContainer walks the 32-byte tag stream directly out
//     of ctx.file (ProfileGOW2::ParseContainer, ported verbatim -- see
//     Gow2Module.cpp).
//   - A .iso: Mounts() declares a MountSpec that returns a mounted
//     Vfs::IsoFileSystem; ParseContainer then reads the GOW2 TOC
//     (GOW2.TOC/GODOFWAR.TOC) through ctx.mountedVfs and opens each
//     referenced PART*.PAK through it too, pushing every PAK it actually
//     reads into ctx.fileTable and stamping the resulting index onto every
//     entry that lives in it (ProfileGOW2::LoadFromArchive +
//     LoadFromArchiveGOW2, ported -- see Gow2Module.cpp).
//
// See task-3-report.md for the full account of what changed for the new
// interface, the Phase-1-vs-this-task error triage, and every decoder
// stubbed here.

#include <Onyx/Modules/GameModule.h>
#include <Onyx/Modules/DecoderRegistry.h>
#include <Onyx/Vfs/IFile.h>

#include <memory>

namespace Onyx::Gow2 {

class Gow2Module final : public Onyx::Modules::IGameModule {
public:
    Onyx::Modules::ModuleInfo  Info() const override;
    Onyx::Modules::ProbeResult Probe(const Onyx::Modules::ProbeInput&) const override;
    void RegisterTypes(Onyx::Types::TypeRegistrar&) override;
    void RegisterDecoders(Onyx::Modules::DecoderRegistry&) override;
    std::vector<Onyx::Modules::MountSpec> Mounts() const override;
    Onyx::Modules::ParseResult ParseContainer(Onyx::Modules::ContainerContext&) override;

private:
    // ── The two ParseContainer bodies (dispatched on ctx.mountedVfs) ──────
    // Non-static (were static pre-Task-4): both stamp AssetEntry::typeId
    // through this module's own minted handles now (RegisterTypes(), below)
    // rather than the legacy Onyx::GameTypes::* externs, so they need an
    // instance. See task-4-report.md's type-convergence section.
    Onyx::Modules::ParseResult ParseWadTagStream(Onyx::Modules::ContainerContext&);
    Onyx::Modules::ParseResult ParseIsoToc(Onyx::Modules::ContainerContext&);

    // ISO-path type assignment (ProfileGOW2's assignSchemaType), same
    // convergence reason as the two functions above -- non-static so it can
    // read this module's minted handles.
    void AssignSchemaType(Onyx::Domain::AssetEntry& entry) const;

    // ── Decoders (bound in RegisterDecoders) ──────────────────────────────

    // TextPlain -> raw bytes as UTF-8 text. Self-contained: reads the
    // entry's own ByteRange through ctx.doc.fileTable[fileIndex], no
    // sibling or material resolution needed.
    static std::optional<Onyx::Modules::DecodedText>
        DecodeTextPlain(Onyx::Modules::DecodeContext&);

    // Model/Mesh/Object -> SceneData is NOT ported. The legacy path
    // (ModelHandler::BuildSceneData, core/types/handlers/ModelHandler.cpp)
    // resolves per-part materials via GOW2MaterialParser and picks a main
    // texture layer per material before it can build a Parsers::SceneData
    // -- exactly the model the brief scopes to Phase 3. Stubbed: reports a
    // diag and returns null rather than a partial port.
    static std::unique_ptr<Onyx::Parsers::SceneData>
        DecodeSceneStub(Onyx::Modules::DecodeContext&);

    // Texture -> TextureData IS ported (fix round 1 corrected an earlier,
    // wrong "DecodeContext has no sibling link" premise this comment used
    // to record here -- it does: GOW2TextureParser::Parse's siblingEntries
    // parameter is satisfied directly by ctx.doc.roots, exactly what the
    // legacy call site (TextureHandler.cpp) passes as wad.entries; bytes
    // come from ctx.doc.fileTable[entry.source.fileIndex]. See
    // task-3-report.md's fix-round addendum.
    static std::unique_ptr<Onyx::Parsers::TextureData>
        DecodeImage(Onyx::Modules::DecodeContext&);

    // ── Types minted in RegisterTypes(), consumed by RegisterDecoders() ───
    // Phase 2 Task 4 closed most of the disconnect Decision 5 (task-3-
    // brief.md) deliberately left open: ParseWadTagStream's own fallback
    // resolution and ParseIsoToc's AssignSchemaType now stamp
    // AssetEntry::typeId from these module-scoped handles, not the legacy
    // Onyx::GameTypes::* externs, so RegisterDecoders()'s wiring (below)
    // fires against entries this module's own ParseContainer produces.
    //
    // ONE residual gap, named precisely rather than papered over: when
    // ParseWadTagStream's Gow::WadTypeRegistry::ResolveByTag(...) call
    // finds a registered IWadTypeHandler (a legacy per-type handler from
    // Source/core/types/handlers/*.cpp, self-registered via
    // REGISTER_GOW_TAG/REGISTER_GOW_TYPE), entry.typeId is still
    // handler->GetId() -- a legacy Onyx::GameTypes::* id, since those
    // handler classes are a separate, still-live subsystem (feeding
    // Onyx::Types::TypeRegistry / AssetHarness::Load()'s BuildSceneData
    // dispatch) this task does not touch. It is unreachable from
    // gowtoolkit_tests today (none of those handler .cpp files are part of
    // APP_TEST_SOURCES, so ResolveByTag always returns nullptr there and
    // every entry falls through to this module's own fallback/m_unknown),
    // so it does not affect the goldens, but it is a real, live disconnect
    // in the app target: converging it needs those handler classes' GetId()
    // to source from a module's minted ids too, which is Phase 3/5
    // territory (materials/handlers, and the CLI), not this task's. See
    // task-4-report.md.
    Onyx::Types::TypeId m_unknown, m_entityCount, m_groupStart, m_groupEnd, m_headerStart,
        m_headerPop, m_instance, m_object, m_model, m_mesh, m_material, m_texture,
        m_gfxData, m_palData, m_animation, m_script, m_light, m_sound, m_collision,
        m_flipbook, m_chunk, m_wadFile, m_vagAudio, m_vpkVideo, m_pssVideo, m_pswVideo,
        m_textPlain;
};

} // namespace Onyx::Gow2
