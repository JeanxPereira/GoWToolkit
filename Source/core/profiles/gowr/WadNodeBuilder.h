#pragma once
#include "core/WadTypes.h"
#include "GOWRTypes.h"
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <functional>

// ── WadNodeBuilder.h ───────────────────────────────────────────────────────
// Converts a flat GOWRFileDesc array (from a parsed WTOC WAD) into a
// hierarchical AssetEntry tree in outWad.entries.
//
// Four passes:
//   Pass 1 — Classify:      assign WadEntryRole + WadBlock to each raw entry
//   Pass 2 — Pair:          merge GPU+CPU texture pairs; fold DCClientGUID
//   Pass 3 — GroupByBlock:  assemble the four functional block folders
//   Pass 4 — Finalize:      set displayName, sort children
//
// All logic is isolated here. GowrModule::ParseContainer only calls Build().
// No other game module, interface, or UI file is affected.

namespace Onyx {

class WadNodeBuilder {
public:
    // Maps a classified WadEntryRole to the Types::TypeId that should be
    // stamped onto the AssetEntry the builder produces for it. Onyx v1.1's
    // module-scoped TypeRegistrar mints its own TypeIds per game module
    // (GowrModule::RegisterTypes) -- this function is how the caller's
    // minted handles reach the tree, instead of WadNodeBuilder hardcoding
    // the legacy Onyx::GameTypes::* externs itself (Phase 2 Task 4
    // convergence; see task-4-report.md). The callee must handle every
    // WadEntryRole this builder can classify, including a sensible
    // "unknown role" fallback -- ToNode/MakeFolder call it unconditionally.
    using RoleToTypeIdFn = std::function<Types::TypeId(WadEntryRole)>;

    // Main entry point. Call once after absOffsets are computed.
    // Clears and repopulates outWad.entries with the final AssetEntry tree.
    void Build(
        const std::vector<GOWRFileDesc>& descs,
        const std::vector<size_t>&       absOffsets,
        const std::string&               wadFilename,
        const RoleToTypeIdFn&             roleToTypeId,
        AssetContainer&                         outWad);

private:
    // ── Internal working entry ─────────────────────────────────────────────
    // Flat mutable working set derived from GOWRFileDesc. Never exposed outside.
    struct RawEntry {
        // From FileDesc
        std::string  name;
        uint32_t     size        = 0;
        uint32_t     offset      = 0;
        uint16_t     group       = 0;
        uint16_t     type        = 0;
        uint8_t      blockBitSet = 0;

        // Derived / builder state
        WadEntryRole role        = WadEntryRole::Unknown;
        WadBlock     block       = WadBlock::Unknown;
        std::string  displayName;
        bool         consumed    = false;  // true = folded into a parent
        int          pairIdx     = -1;     // index of paired CPU entry (TextureGpu only)
    };

    std::vector<RawEntry> m_entries;
    std::string           m_wadFilename;
    RoleToTypeIdFn        m_roleToTypeId;

    // ── The four passes ────────────────────────────────────────────────────
    void Pass1_Classify();
    void Pass2_Pair();
    void Pass3_GroupByBlock(AssetContainer& outWad);
    void Pass4_Finalize(AssetContainer& outWad);

    // ── Helpers ────────────────────────────────────────────────────────────

    // Convert "ANMX_R_Fox00" → "ANMX → ANMX_Shared_Fox00"
    static std::string MakeSharedWadName(const std::string& entryName);

    // Build a AssetEntry leaf node from a RawEntry. Non-static (was static
    // pre-Task-4): reads m_roleToTypeId to stamp typeId, so it needs an
    // instance.
    AssetEntry ToNode(const RawEntry& r, const std::string& wadFilename) const;

    // Strip trailing content hash from texture display name
    // "TX_name_slot_1D293ECA4DE04637" → "TX_name_slot"
    static std::string StripTextureHash(const std::string& name);

    // Extract FX context string from a go*/goProto* particle entry name
    // "goProtofox00_envRaceIntro_dust_landing" → "envRaceIntro_dust_landing"
    static std::string ExtractGoContext(const std::string& name);

    // Build a synthetic virtual folder node (block, shader group, FX group, etc.)
    AssetEntry MakeFolder(
        const std::string& name,
        WadEntryRole       role,
        WadBlock           block = WadBlock::Unknown) const;

    // Sort-key for asset ordering within the Assets block
    static int AssetSortKey(WadEntryRole role);
};

} // namespace Onyx
