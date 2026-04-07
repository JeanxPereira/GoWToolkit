#pragma once
#include "../../WadTypes.h"
#include "GOWRTypes.h"
#include <vector>
#include <string>
#include <map>
#include <cstdint>

// ── WadNodeBuilder.h ───────────────────────────────────────────────────────
// Converts a flat GOWRFileDesc array (from a parsed WTOC WAD) into a
// hierarchical ParsedEntry tree in outWad.entries.
//
// Four passes:
//   Pass 1 — Classify:      assign WadEntryRole + WadBlock to each raw entry
//   Pass 2 — Pair:          merge GPU+CPU texture pairs; fold DCClientGUID
//   Pass 3 — GroupByBlock:  assemble the four functional block folders
//   Pass 4 — Finalize:      set displayName, sort children
//
// All logic is isolated here. ProfileGOWR::ParseWad only calls Build().
// No other game profile, interface, or UI file is affected.

namespace GOW {

class WadNodeBuilder {
public:
    // Main entry point. Call once after absOffsets are computed.
    // Clears and repopulates outWad.entries with the final ParsedEntry tree.
    void Build(
        const std::vector<GOWRFileDesc>& descs,
        const std::vector<size_t>&       absOffsets,
        const std::string&               wadFilename,
        OpenWad&                         outWad);

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
        std::string  schemaType;

        // Derived / builder state
        WadEntryRole role        = WadEntryRole::Unknown;
        WadBlock     block       = WadBlock::Unknown;
        std::string  displayName;
        bool         consumed    = false;  // true = folded into a parent
        int          pairIdx     = -1;     // index of paired CPU entry (TextureGpu only)
    };

    std::vector<RawEntry> m_entries;
    std::string           m_wadFilename;

    // ── The four passes ────────────────────────────────────────────────────
    void Pass1_Classify();
    void Pass2_Pair();
    void Pass3_GroupByBlock(OpenWad& outWad);
    void Pass4_Finalize(OpenWad& outWad);

    // ── Helpers ────────────────────────────────────────────────────────────

    // Classify a single entry by name + size → WadEntryRole
    static WadEntryRole ClassifyByName(const std::string& name, uint32_t size);

    // Convert "ANMX_R_Fox00" → "ANMX → ANMX_Shared_Fox00"
    static std::string MakeSharedWadName(const std::string& entryName);

    // Build a ParsedEntry leaf node from a RawEntry
    static ParsedEntry ToNode(const RawEntry& r, const std::string& wadFilename);

    // Strip trailing content hash from texture display name
    // "TX_name_slot_1D293ECA4DE04637" → "TX_name_slot"
    static std::string StripTextureHash(const std::string& name);

    // Extract FX context string from a go*/goProto* particle entry name
    // "goProtofox00_envRaceIntro_dust_landing" → "envRaceIntro_dust_landing"
    static std::string ExtractGoContext(const std::string& name);

    // Build a synthetic virtual folder node (block, shader group, FX group, etc.)
    ParsedEntry MakeFolder(
        const std::string& name,
        const std::string& schemaType,
        WadEntryRole       role,
        WadBlock           block = WadBlock::Unknown) const;

    // Sort-key for asset ordering within the Assets block
    static int AssetSortKey(WadEntryRole role);
};

} // namespace GOW
