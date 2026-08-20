#include "WadNodeBuilder.h"
#include "core/profiles/gowr/GowrTaxonomy.h"
#include <Onyx/Types/TypeCatalog.h>
#include <algorithm>
#include <cctype>
#include <cstring>

// ── WadNodeBuilder.cpp ─────────────────────────────────────────────────────
// Four-pass WAD entry tree builder for God of War Ragnarök.
// See WadNodeBuilder.h for architecture overview.

namespace Onyx {

// Onyx v1.1 removed AssetEntry::profileTag (and all per-entry storage), so
// role is no longer stored — it is reclassified on demand from the entry's
// own name + size via Gowr::Classify(). This exactly reproduces the role
// every *leaf* entry got in Pass1_Classify (Classify() calls the same
// ClassifyByName the builder does).
//
// It does NOT reproduce the four synthetic block-folder roles (ManifestBlock/
// ShaderBlock/AssetBlock/ParticleBlock) or ShaderGroup/FxGroup: those were
// assigned directly by MakeFolder(), never derived from a name pattern, so
// Classify() correctly has no way to recover them (their MakeFolder-built
// names/sizes don't match any ClassifyByName rule and fall to Unknown).
// Pass4_Finalize below identifies those synthetic folders structurally
// instead of through GetRole(), which preserves the original behaviour
// exactly rather than silently losing it.
static Gowr::WadEntryRole GetRole(const AssetEntry& e) {
    return Gowr::Classify(e).role;
}

// ═══════════════════════════════════════════════════════════════════════════
// Public entry point
// ═══════════════════════════════════════════════════════════════════════════

void WadNodeBuilder::Build(
    const std::vector<GOWRFileDesc>& descs,
    const std::vector<size_t>&       absOffsets,
    const std::string&               wadFilename,
    const RoleToTypeIdFn&             roleToTypeId,
    AssetContainer&                         outWad)
{
    m_wadFilename  = wadFilename;
    m_roleToTypeId = roleToTypeId;
    m_entries.clear();
    m_entries.reserve(descs.size());

    for (size_t i = 0; i < descs.size(); ++i) {
        RawEntry r;
        r.name        = std::string(descs[i].name,
                            strnlen(descs[i].name, sizeof(descs[i].name)));
        r.size        = descs[i].size;
        r.offset      = static_cast<uint32_t>(absOffsets[i]);
        r.group       = descs[i].group;
        r.type        = descs[i].type;
        r.blockBitSet = descs[i].blockBitSet;
        m_entries.push_back(std::move(r));
    }

    Pass1_Classify();
    Pass2_Pair();
    Pass3_GroupByBlock(outWad);
    Pass4_Finalize(outWad);
}

// ═══════════════════════════════════════════════════════════════════════════
// Pass 1 — Classify
// Assign WadEntryRole and WadBlock to every RawEntry.
// ═══════════════════════════════════════════════════════════════════════════

void WadNodeBuilder::Pass1_Classify() {
    WadBlock currentBlock = WadBlock::Manifest;

    for (size_t i = 0; i < m_entries.size(); ++i) {
        auto& e = m_entries[i];

        // Classify role from name + size (Gowr::ClassifyByName is also what
        // Gowr::Classify() calls to reconstruct role from an AssetEntry —
        // single source of truth, see GowrTaxonomy.cpp).
        e.role  = Gowr::ClassifyByName(e.name, e.size);

        // Assign current block (may be overridden by transition logic below)
        e.block = currentBlock;

        // ── Block state machine ──────────────────────────────────────────
        switch (currentBlock) {
            case WadBlock::Manifest:
                // PopHeap marks end of Manifest → transition to Shaders
                if (e.role == WadEntryRole::Sentinel && e.name == "PopHeap") {
                    currentBlock = WadBlock::Shaders;
                }
                break;

            case WadBlock::Shaders:
                // Any non-shader, non-sentinel entry ends the Shaders block
                if (e.role != WadEntryRole::ShaderContainer &&
                    e.role != WadEntryRole::ShaderVertex    &&
                    e.role != WadEntryRole::ShaderPixel     &&
                    e.role != WadEntryRole::ShaderHull      &&
                    e.role != WadEntryRole::ShaderDomain    &&
                    e.role != WadEntryRole::ShaderCompute   &&
                    e.role != WadEntryRole::ShaderLibrary   &&
                    e.role != WadEntryRole::Sentinel)
                {
                    // Check whether we jump straight to Particles (unlikely, but safe)
                    currentBlock = (e.role == WadEntryRole::ParticleEmitter)
                                 ? WadBlock::Particles
                                 : WadBlock::Assets;
                    e.block = currentBlock;
                }
                break;

            case WadBlock::Assets:
                // First PEM_emit_* entry begins the Particles block
                if (e.role == WadEntryRole::ParticleEmitter ||
                    e.role == WadEntryRole::ParticleSystem)
                {
                    currentBlock = WadBlock::Particles;
                    e.block      = WadBlock::Particles;
                }
                break;

            case WadBlock::Particles:
                // Everything until end of file stays in Particles
                break;

            default:
                break;
        }
    }
}

// ClassifyByName moved to Gowr::ClassifyByName (GowrTaxonomy.cpp) so
// Gowr::Classify() can call the exact same rules when reconstructing role
// from an AssetEntry — see the comment on Pass1_Classify's call site above.

// ═══════════════════════════════════════════════════════════════════════════
// Pass 2 — Pair
// Merge GPU+CPU texture pairs; fold DCClientGUID entries.
// ═══════════════════════════════════════════════════════════════════════════

void WadNodeBuilder::Pass2_Pair() {
    // ── Texture pairing ───────────────────────────────────────────────────
    // For each TextureGpu, find the nearest forward TextureCpu with the same name.
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].consumed) continue;
        if (m_entries[i].role != WadEntryRole::TextureGpu) continue;

        for (size_t j = i + 1; j < m_entries.size(); ++j) {
            if (m_entries[j].consumed) continue;
            if (m_entries[j].role == WadEntryRole::TextureCpu &&
                m_entries[j].name == m_entries[i].name)
            {
                m_entries[i].pairIdx     = static_cast<int>(j);
                m_entries[j].consumed    = true;  // CPU will become a child of GPU
                break;
            }
        }
    }

    // ── DCClientGUID folding ──────────────────────────────────────────────
    // DCClientGUID is internal engine plumbing (registration handle for wadContext).
    // It carries no displayable content and always precedes the actual asset entry.
    // Mark them consumed so they are omitted from the tree entirely.
    for (auto& e : m_entries) {
        if (e.role == WadEntryRole::ClientGuid) {
            e.consumed = true;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Pass 3 — GroupByBlock
// Build the four top-level block folders and populate their children.
// ═══════════════════════════════════════════════════════════════════════════

void WadNodeBuilder::Pass3_GroupByBlock(AssetContainer& outWad) {

    // ── MANIFEST ──────────────────────────────────────────────────────────
    {
        AssetEntry manifestFolder = MakeFolder(
            "Manifest",
            WadEntryRole::ManifestBlock, WadBlock::Manifest);

        for (auto& e : m_entries) {
            if (e.block != WadBlock::Manifest || e.consumed) continue;
            if (e.role  == WadEntryRole::Sentinel)            continue; // omit PopHeap

            AssetEntry node = ToNode(e, m_wadFilename);
            if (e.role == WadEntryRole::SharedWadRef)
                node.displayName = MakeSharedWadName(e.name);

            manifestFolder.children.push_back(std::move(node));
        }

        if (!manifestFolder.children.empty())
            outWad.entries.push_back(std::move(manifestFolder));
    }

    // ── SHADERS ───────────────────────────────────────────────────────────
    {
        AssetEntry shadersFolder = MakeFolder(
            "Shaders",
            WadEntryRole::ShaderBlock, WadBlock::Shaders);

        AssetEntry vsFolder = MakeFolder(
            "[Vertex Shaders]", WadEntryRole::ShaderGroup);
        AssetEntry psFolder = MakeFolder(
            "[Pixel Shaders]", WadEntryRole::ShaderGroup);
        AssetEntry hsFolder = MakeFolder(
            "[Hull Shaders]", WadEntryRole::ShaderGroup);
        AssetEntry dsFolder = MakeFolder(
            "[Domain Shaders]", WadEntryRole::ShaderGroup);
        AssetEntry csFolder = MakeFolder(
            "[Compute Shaders]", WadEntryRole::ShaderGroup);
        AssetEntry lsFolder = MakeFolder(
            "[Library Shaders]", WadEntryRole::ShaderGroup);
        AssetEntry containerFolder = MakeFolder(
            "[Containers]", WadEntryRole::ShaderGroup);

        // Group VS by prefix before _vs_; PS by prefix before _ps_, etc
        std::map<std::string, AssetEntry> vsGroups;
        std::map<std::string, AssetEntry> psGroups;
        std::map<std::string, AssetEntry> hsGroups;
        std::map<std::string, AssetEntry> dsGroups;
        std::map<std::string, AssetEntry> csGroups;
        std::map<std::string, AssetEntry> lsGroups;

        for (auto& e : m_entries) {
            if (e.block != WadBlock::Shaders || e.consumed) continue;
            if (e.role  == WadEntryRole::Sentinel)           continue;

            if (e.role == WadEntryRole::ShaderVertex) {
                auto pos    = e.name.find("_vs_");
                std::string prefix = (pos != std::string::npos)
                                   ? e.name.substr(0, pos)
                                   : e.name;
                if (vsGroups.find(prefix) == vsGroups.end())
                    vsGroups[prefix] = MakeFolder(prefix,
                                                  WadEntryRole::ShaderGroup);
                vsGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderPixel) {
                auto pos    = e.name.find("_ps_");
                std::string prefix = (pos != std::string::npos)
                                   ? e.name.substr(0, pos)
                                   : e.name;
                if (psGroups.find(prefix) == psGroups.end())
                    psGroups[prefix] = MakeFolder(prefix,
                                                  WadEntryRole::ShaderGroup);
                psGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderHull) {
                auto pos    = e.name.find("_hs_");
                std::string prefix = (pos != std::string::npos) ? e.name.substr(0, pos) : e.name;
                if (hsGroups.find(prefix) == hsGroups.end())
                    hsGroups[prefix] = MakeFolder(prefix, WadEntryRole::ShaderGroup);
                hsGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderDomain) {
                auto pos    = e.name.find("_ds_");
                std::string prefix = (pos != std::string::npos) ? e.name.substr(0, pos) : e.name;
                if (dsGroups.find(prefix) == dsGroups.end())
                    dsGroups[prefix] = MakeFolder(prefix, WadEntryRole::ShaderGroup);
                dsGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderCompute) {
                auto pos    = e.name.find("_cs_");
                std::string prefix = (pos != std::string::npos) ? e.name.substr(0, pos) : e.name;
                if (csGroups.find(prefix) == csGroups.end())
                    csGroups[prefix] = MakeFolder(prefix, WadEntryRole::ShaderGroup);
                csGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderLibrary) {
                auto pos    = e.name.find("_ls_");
                std::string prefix = (pos != std::string::npos) ? e.name.substr(0, pos) : e.name;
                if (lsGroups.find(prefix) == lsGroups.end())
                    lsGroups[prefix] = MakeFolder(prefix, WadEntryRole::ShaderGroup);
                lsGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderContainer) {
                containerFolder.children.push_back(ToNode(e, m_wadFilename));
            }
        }

        // Flatten single-variant shader groups directly into the parent folder
        auto flushGroups = [](std::map<std::string, AssetEntry>& groups,
                              AssetEntry& targetFolder)
        {
            for (auto& [prefix, group] : groups) {
                if (group.children.size() == 1) {
                    // Only one variant — no need for a sub-folder
                    targetFolder.children.push_back(std::move(group.children[0]));
                } else {
                    targetFolder.children.push_back(std::move(group));
                }
            }
        };

        flushGroups(vsGroups, vsFolder);
        flushGroups(psGroups, psFolder);
        flushGroups(hsGroups, hsFolder);
        flushGroups(dsGroups, dsFolder);
        flushGroups(csGroups, csFolder);
        flushGroups(lsGroups, lsFolder);

        if (!vsFolder.children.empty())        shadersFolder.children.push_back(std::move(vsFolder));
        if (!psFolder.children.empty())        shadersFolder.children.push_back(std::move(psFolder));
        if (!hsFolder.children.empty())        shadersFolder.children.push_back(std::move(hsFolder));
        if (!dsFolder.children.empty())        shadersFolder.children.push_back(std::move(dsFolder));
        if (!csFolder.children.empty())        shadersFolder.children.push_back(std::move(csFolder));
        if (!lsFolder.children.empty())        shadersFolder.children.push_back(std::move(lsFolder));
        if (!containerFolder.children.empty()) shadersFolder.children.push_back(std::move(containerFolder));

        if (!shadersFolder.children.empty())
            outWad.entries.push_back(std::move(shadersFolder));
    }

    // ── ASSETS ────────────────────────────────────────────────────────────
    {
        AssetEntry assetsFolder = MakeFolder(
            "Assets",
            WadEntryRole::AssetBlock, WadBlock::Assets);

        AssetEntry lodFolder = MakeFolder(
            "[LOD Bindings]",
            WadEntryRole::LodBinding, WadBlock::Assets);

        for (auto& e : m_entries) {
            if (e.block != WadBlock::Assets || e.consumed) continue;

            if (e.role == WadEntryRole::TextureGpu) {
                // TexturePair flat node — GPU + CPU sub-entries are internal
                // streaming plumbing with no standalone view, so we hide them.
                //
                // This used to relabel the node's stored role TextureGpu ->
                // TexturePair via profileTag (typeId was already TexturePair
                // either way — RoleToTypeId collapses all three texture
                // roles onto it). That storage is gone in Onyx v1.1; the
                // relabel is dropped rather than reproduced because it was
                // unobservable: every consumer of role (RoleVisuals'
                // IconForRole/ColorForRole, AssetSortKey) groups
                // TexturePair/TextureGpu/TextureCpu identically, so nothing
                // downstream can tell the difference. Gowr::Classify() now
                // reconstructs TextureGpu for this node from its own name +
                // size, which is the same outcome by a different name.
                AssetEntry pairNode = ToNode(e, m_wadFilename);
                pairNode.displayName = StripTextureHash(e.name);
                assetsFolder.children.push_back(std::move(pairNode));

            } else if (e.role == WadEntryRole::LodBinding) {
                lodFolder.children.push_back(ToNode(e, m_wadFilename));

            } else {
                AssetEntry node = ToNode(e, m_wadFilename);
                // MaterialRef (sz=0) is a back-reference — prefix with arrow
                if (e.role == WadEntryRole::MaterialRef)
                    node.displayName = "-> " + e.name;
                assetsFolder.children.push_back(std::move(node));
            }
        }

        // Prepend LOD bindings folder if non-empty
        if (!lodFolder.children.empty())
            assetsFolder.children.insert(
                assetsFolder.children.begin(), std::move(lodFolder));

        if (!assetsFolder.children.empty())
            outWad.entries.push_back(std::move(assetsFolder));
    }

    // ── PARTICLES ─────────────────────────────────────────────────────────
    // Group strategy: scan sequentially; each go* (GameObjectInst) entry marks
    // the end of one FX context group. All preceding ungrouped PEM/PTC/MAT entries
    // belong to that context.
    {
        AssetEntry particlesFolder = MakeFolder(
            "Particles",
            WadEntryRole::ParticleBlock, WadBlock::Particles);

        // Collect indices of non-consumed Particle block entries in order
        std::vector<size_t> pidx;
        for (size_t i = 0; i < m_entries.size(); ++i) {
            if (m_entries[i].block == WadBlock::Particles && !m_entries[i].consumed)
                pidx.push_back(i);
        }

        // Walk pidx; each GameObjectInst closes a group
        size_t groupStart = 0;
        for (size_t pi = 0; pi < pidx.size(); ++pi) {
            auto& e = m_entries[pidx[pi]];

            if (e.role == WadEntryRole::GameObjectInst) {
                // Extract context from the go* entry name
                std::string ctx = ExtractGoContext(e.name);

                AssetEntry fxGroup = MakeFolder(
                    ctx.empty() ? ("FX_" + e.name) : ctx,
                    WadEntryRole::FxGroup, WadBlock::Particles);

                for (size_t k = groupStart; k <= pi; ++k)
                    fxGroup.children.push_back(ToNode(m_entries[pidx[k]], m_wadFilename));

                particlesFolder.children.push_back(std::move(fxGroup));
                groupStart = pi + 1;
            }
        }

        // Any remaining entries after the last go* are singletons
        for (size_t pi = groupStart; pi < pidx.size(); ++pi)
            particlesFolder.children.push_back(
                ToNode(m_entries[pidx[pi]], m_wadFilename));

        if (!particlesFolder.children.empty())
            outWad.entries.push_back(std::move(particlesFolder));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Pass 4 — Finalize
// Set missing displayNames; sort children within each block.
// ═══════════════════════════════════════════════════════════════════════════

// Sort priority within the Assets block (lower = earlier)
int WadNodeBuilder::AssetSortKey(WadEntryRole role) {
    switch (role) {
        case WadEntryRole::TexturePair:
        case WadEntryRole::TextureGpu:
        case WadEntryRole::TextureCpu:        return 0;
        case WadEntryRole::Material:
        case WadEntryRole::MaterialRef:       return 1;
        case WadEntryRole::LodBinding:        return 2;
        case WadEntryRole::MeshGpu:
        case WadEntryRole::MeshDefn:          return 3;
        case WadEntryRole::Model:             return 4;
        case WadEntryRole::AnimClip:          return 5;
        case WadEntryRole::GameObjectProto:
        case WadEntryRole::GameObjectInst:
        case WadEntryRole::GameObjectOverride: return 6;
        case WadEntryRole::SoundEmitter:      return 7;
        default:                              return 8;
    }
}

void WadNodeBuilder::Pass4_Finalize(AssetContainer& outWad) {
    // outWad.entries holds only the (up to) four block folders Pass3 built —
    // manifestFolder/shadersFolder/assetsFolder/particlesFolder — each
    // pushed under one of these exact literal names and nothing else. Their
    // synthetic roles (ManifestBlock/ShaderBlock/AssetBlock/ParticleBlock)
    // were never derivable from a name pattern in the first place (MakeFolder
    // assigns them directly, ClassifyByName has no rule that would produce
    // them), so identify them by that name rather than through GetRole().
    for (auto& blockNode : outWad.entries) {

        // ── Manifest: already in order; no sort needed ─────────────────
        if (blockNode.name == "Manifest")
            continue;

        // ── Shaders: sort groups alphabetically ─────────────────────
        if (blockNode.name == "Shaders") {
            for (auto& subFolder : blockNode.children) {
                if (subFolder.name == "[Vertex Shaders]" ||
                    subFolder.name == "[Pixel Shaders]" ||
                    subFolder.name == "[Hull Shaders]" ||
                    subFolder.name == "[Domain Shaders]" ||
                    subFolder.name == "[Compute Shaders]" ||
                    subFolder.name == "[Library Shaders]")
                {
                    std::sort(subFolder.children.begin(), subFolder.children.end(),
                        [](const AssetEntry& a, const AssetEntry& b) {
                            return a.name < b.name;
                        });
                    // Sort variants within each group by FLAGS suffix
                    for (auto& shaderGroup : subFolder.children) {
                        if (!shaderGroup.children.empty()) {
                            std::sort(shaderGroup.children.begin(), shaderGroup.children.end(),
                                [](const AssetEntry& a, const AssetEntry& b) {
                                    return a.name < b.name;
                                });
                        }
                    }
                }
            }
            continue;
        }

        // ── Assets: textures → materials → mesh/model → gameobj → audio ─
        if (blockNode.name == "Assets") {
            std::stable_sort(blockNode.children.begin(), blockNode.children.end(),
                [](const AssetEntry& a, const AssetEntry& b) {
                    int ka = AssetSortKey(GetRole(a));
                    int kb = AssetSortKey(GetRole(b));
                    if (ka != kb) return ka < kb;
                    // Within same category: sort by display name (falling back to name)
                    const std::string& na = a.displayName.empty() ? a.name : a.displayName;
                    const std::string& nb = b.displayName.empty() ? b.name : b.displayName;
                    return na < nb;
                });
            continue;
        }

        // ── Particles: FX groups sorted alphabetically ─────────────────
        if (blockNode.name == "Particles") {
            std::sort(blockNode.children.begin(), blockNode.children.end(),
                [](const AssetEntry& a, const AssetEntry& b) {
                    // Folders before singletons
                    bool aFolder = !a.children.empty();
                    bool bFolder = !b.children.empty();
                    if (aFolder != bFolder) return aFolder > bFolder;
                    return a.name < b.name;
                });
            // Within each FxGroup: emitters → systems → material refs → protos → insts
            // blockNode.children here is either an FxGroup folder (MakeFolder,
            // always >=1 child — see Pass3_GroupByBlock) or a singleton leaf
            // from ToNode (which never populates .children), so a non-empty
            // children vector identifies an FxGroup exactly as GetRole() used
            // to (FxGroup names are arbitrary FX contexts extracted from a
            // go* name, not a pattern Classify() could recognise).
            for (auto& fxGroup : blockNode.children) {
                if (!fxGroup.children.empty()) {
                    std::stable_sort(fxGroup.children.begin(), fxGroup.children.end(),
                        [](const AssetEntry& a, const AssetEntry& b) {
                            auto fxKey = [](WadEntryRole r) {
                                switch (r) {
                                    case WadEntryRole::ParticleEmitter:    return 0;
                                    case WadEntryRole::ParticleSystem:     return 1;
                                    case WadEntryRole::MaterialRef:        return 2;
                                    case WadEntryRole::GameObjectProto:    return 3;
                                    case WadEntryRole::GameObjectInst:     return 4;
                                    default:                               return 5;
                                }
                            };
                            return fxKey(GetRole(a)) < fxKey(GetRole(b));
                        });
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

// RoleToTypeId is gone: the mapping now lives in the caller (GowrModule::
// RegisterTypes' minted handles), reached here via m_roleToTypeId — see
// WadNodeBuilder.h's RoleToTypeIdFn doc comment and task-4-report.md's type-
// convergence section.

AssetEntry WadNodeBuilder::ToNode(const RawEntry& r, const std::string& wadFilename) const {
    AssetEntry e;
    e.name        = r.name;
    e.source.size   = r.size;
    e.source.offset = r.offset;
    e.wadName     = wadFilename;
    e.typeId      = m_roleToTypeId(r.role);
    e.kind        = Types::KindOf(e.typeId);
    e.displayName = r.displayName;
    // No profileTag write: Onyx v1.1 has no per-entry storage. role is
    // reconstructed on demand by Gowr::Classify(e) from e.name + e.source.size.
    return e;
}

AssetEntry WadNodeBuilder::MakeFolder(
    const std::string& name,
    WadEntryRole       role,
    WadBlock           block) const
{
    AssetEntry f;
    f.name       = name;
    f.typeId     = m_roleToTypeId(role);
    f.wadName    = m_wadFilename;
    f.kind       = Types::KindOf(f.typeId);
    // No profileTag write: see ToNode() above. Synthetic folder roles like
    // this one are not recoverable from (name, size) — Pass4_Finalize
    // identifies these folders structurally instead of via GetRole/Classify.
    return f;
}

// "ANMX_R_Fox00" → "ANMX → ANMX_Shared_Fox00"
std::string WadNodeBuilder::MakeSharedWadName(const std::string& entryName) {
    auto pos = entryName.find("_R_");
    if (pos == std::string::npos) return entryName;

    std::string prefix     = entryName.substr(0, pos);    // e.g. "ANMX"
    std::string base       = entryName.substr(pos + 3);   // e.g. "Fox00"
    std::string sharedName = prefix + "_Shared_" + base;  // "ANMX_Shared_Fox00"
    return prefix + " -> " + sharedName;               // "ANMX -> ANMX_Shared_Fox00"
}

// Strip trailing hex content hash from a texture name.
// "TX_angrboda_fox00_head_gen_0d_1D293ECA4DE04637" → "TX_angrboda_fox00_head_gen_0d"
std::string WadNodeBuilder::StripTextureHash(const std::string& name) {
    auto lastUs = name.rfind('_');
    if (lastUs == std::string::npos) return name;

    std::string suffix = name.substr(lastUs + 1);
    if (suffix.size() < 8) return name;  // too short to be a hash

    bool isHex = true;
    for (char c : suffix) {
        if (!isxdigit((unsigned char)c)) { isHex = false; break; }
    }
    return isHex ? name.substr(0, lastUs) : name;
}

// Extract FX context from a go* or goProto* name.
// "goProtofox00_envRaceIntro_dust_landing" → "envRaceIntro_dust_landing"
// "gofox00_envraceintro_dust_landing"      → "envraceintro_dust_landing"
// Returns empty string if no context can be extracted.
std::string WadNodeBuilder::ExtractGoContext(const std::string& name) {
    // Strip leading prefix (goProto* or go*)
    size_t prefixEnd = 0;
    if (name.rfind("goProto", 0) == 0) {
        prefixEnd = 7;  // len("goProto")
    } else if (name.rfind("go", 0) == 0) {
        prefixEnd = 2;  // len("go")
    } else {
        return "";
    }

    // Skip the base name (everything up to the first underscore after the prefix)
    auto us = name.find('_', prefixEnd);
    if (us == std::string::npos) return "";  // no underscore → no context

    return name.substr(us + 1);  // e.g. "envRaceIntro_dust_landing"
}

} // namespace Onyx
