#pragma once

// Projects a Workspace Document onto the legacy Domain::AssetContainer that
// the ITypeHandler / ViewerRegistry / parser layers still speak.
//
// Onyx v1.1 keeps AssetContainer for exactly this reason (Onyx/Domain/Wad.h),
// and the same five-line projection had been written out four separate times
// -- AssetHarness::LoadContainer, WadBrowser::BridgeFor, GowrModule::DecodeMesh
// and the --gui startup path -- each with its own copy of the fileIndex
// reasoning below. One of them getting it wrong is not hypothetical: the
// harness originally hardcoded slot 0 and read a compressed GOWR wad's offsets
// against the still-compressed bytes.

#include <Onyx/Domain/Wad.h>
#include <Onyx/Modules/Workspace.h>

namespace Onyx::Gow {

/// Builds the AssetContainer view of `doc`.
///
/// fileSource resolves through roots[0]'s own `source.fileIndex` rather than a
/// hardcoded slot 0: a compressed GOWR wad pushes its LZ4-decompressed buffer
/// into a later file-table slot, and GowrModule stamps that same index on every
/// node it produces, so roots[0] names the right slot for the whole tree.
///
/// Still NOT exact for a mounted GOW2 ISO, where ParseIsoToc can stamp
/// different entries with different fileIndexes (one per PART*.PAK) while
/// AssetContainer holds a single fileSource. An entry whose fileIndex differs
/// from roots[0]'s reads the wrong bytes through this bridge. That is
/// AssetContainer's structural limit, not something the projection can fix.
inline Onyx::Domain::AssetContainer MakeContainerBridge(const Onyx::Modules::Document& doc) {
    Onyx::Domain::AssetContainer wad;
    const uint32_t primary = doc.roots.empty() ? 0u : doc.roots[0].source.fileIndex;
    wad.filename   = doc.path.filename().string();
    wad.fullPath   = doc.path.string();
    wad.fileSource = primary < doc.fileTable.size() ? doc.fileTable[primary] : doc.file;
    wad.entries    = doc.roots;
    return wad;
}

} // namespace Onyx::Gow
