#include "WadNodeBuilder.h"
#include <algorithm>
#include <cctype>
#include <cstring>

// ── WadNodeBuilder.cpp ─────────────────────────────────────────────────────
// Four-pass WAD entry tree builder for God of War Ragnarök.
// See WadNodeBuilder.h for architecture overview.

namespace GOW {

// ═══════════════════════════════════════════════════════════════════════════
// Public entry point
// ═══════════════════════════════════════════════════════════════════════════

void WadNodeBuilder::Build(
    const std::vector<GOWRFileDesc>& descs,
    const std::vector<size_t>&       absOffsets,
    const std::string&               wadFilename,
    OpenWad&                         outWad)
{
    m_wadFilename = wadFilename;
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
        r.schemaType  = GOWRTypeToString(descs[i].type);
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

        // Classify role from name + size
        e.role  = ClassifyByName(e.name, e.size);

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

// ── ClassifyByName ─────────────────────────────────────────────────────────
// Priority-ordered pattern matching. Rules mirror §6.2 of the planning spec.

WadEntryRole WadNodeBuilder::ClassifyByName(const std::string& name, uint32_t size) {
    if (name.empty()) return WadEntryRole::Unknown;

    // ── Sentinels ────────────────────────────────────────────────────────
    if (name == "PopHeap" || name == "autopad")
        return WadEntryRole::Sentinel;

    // ── SharedWadRef: ^[A-Z]+X_R_  (e.g. TXRX_R_Fox00, ANMX_R_Fox00) ──
    // Must be checked BEFORE WadIdentity to avoid matching WAD_R_ (no X before _R_)
    {
        size_t i = 0;
        while (i < name.size() && isupper((unsigned char)name[i])) ++i;
        // i >= 2 ensures at least 2 uppercase chars; name[i-1] must be 'X'
        if (i >= 2 && name[i - 1] == 'X' &&
            name.size() > i + 2 &&
            name[i] == '_' && name[i + 1] == 'R' && name[i + 2] == '_')
        {
            return WadEntryRole::SharedWadRef;
        }
    }

    // ── WAD identity: starts with WAD_ ───────────────────────────────────
    if (name.rfind("WAD_", 0) == 0)
        return WadEntryRole::WadIdentity;

    // ── Shader container: starts with 0x ─────────────────────────────────
    if (name.rfind("0x", 0) == 0)
        return WadEntryRole::ShaderContainer;

    // ── Shaders: _vs_ / _ps_ anywhere in name ────────────────────────────
    if (name.find("_vs_") != std::string::npos)
        return WadEntryRole::ShaderVertex;
    if (name.find("_ps_") != std::string::npos)
        return WadEntryRole::ShaderPixel;
    if (name.find("_hs_") != std::string::npos)
        return WadEntryRole::ShaderHull;
    if (name.find("_ds_") != std::string::npos)
        return WadEntryRole::ShaderDomain;
    if (name.find("_cs_") != std::string::npos)
        return WadEntryRole::ShaderCompute;
    if (name.find("_ls_") != std::string::npos)
        return WadEntryRole::ShaderLibrary;

    // Named vertex shaders without hash prefix
    if (name.rfind("depth_vs",  0) == 0 ||
        name.rfind("depvl_vs",  0) == 0 ||
        name.rfind("opaque_vs", 0) == 0 ||
        name.rfind("transp_vs", 0) == 0)
    {
        return WadEntryRole::ShaderVertex;
    }

    // ── Animation ────────────────────────────────────────────────────────
    if (name.rfind("ANM_", 0) == 0)
        return WadEntryRole::AnimClip;

    // ── Textures (discriminated by size) ─────────────────────────────────
    if (name.rfind("TX_", 0) == 0)
        return (size >= 1024) ? WadEntryRole::TextureGpu : WadEntryRole::TextureCpu;

    // ── Materials (discriminated by size) ────────────────────────────────
    if (name.rfind("MAT_", 0) == 0)
        return (size > 0) ? WadEntryRole::Material : WadEntryRole::MaterialRef;

    // ── LOD binding table: matches /^\d+_\d+_\d+$/ ───────────────────────
    {
        bool isLod       = !name.empty();
        int  underscores = 0;
        for (char c : name) {
            if      (c == '_')             ++underscores;
            else if (!isdigit((unsigned char)c)) { isLod = false; break; }
        }
        if (isLod && underscores == 2)
            return WadEntryRole::LodBinding;
    }

    // ── Mesh (order matters: MeshGpu before MeshDefn) ────────────────────
    if (name.rfind("MG_", 0) == 0) {
        if (name.size() > 4 && name.substr(name.size() - 4) == "_gpu")
            return WadEntryRole::MeshGpu;
        return WadEntryRole::MeshDefn;   // MG_ without _gpu = mesh group def
    }
    if (name.rfind("MESH_", 0) == 0)
        return WadEntryRole::MeshDefn;
    if (name.rfind("MDL_", 0) == 0)
        return WadEntryRole::Model;

    // ── Game objects (order matters: Override > Proto > Inst) ────────────
    if (name.rfind("goProto", 0) == 0)
        return WadEntryRole::GameObjectProto;

    if (name.size() >= 13 &&
        name.substr(name.size() - 13) == "_overrideInst")
    {
        return WadEntryRole::GameObjectOverride;
    }

    // Plain go* instance: starts with "go" followed by a lowercase letter
    // (guards against matching "goProto" again and other non-game-object "go" names)
    if (name.size() > 2 &&
        name[0] == 'g' && name[1] == 'o' &&
        islower((unsigned char)name[2]))
    {
        return WadEntryRole::GameObjectInst;
    }

    // ── Audio ─────────────────────────────────────────────────────────────
    if (name.rfind("SEMW_", 0) == 0)
        return WadEntryRole::SoundEmitter;

    // ── Particle FX ───────────────────────────────────────────────────────
    if (name == "DCClientGUID")
        return WadEntryRole::ClientGuid;
    if (name.rfind("PEM_emit_", 0) == 0)
        return WadEntryRole::ParticleEmitter;
    if (name.rfind("PTC_part_", 0) == 0)
        return WadEntryRole::ParticleSystem;

    return WadEntryRole::Unknown;
}

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

void WadNodeBuilder::Pass3_GroupByBlock(OpenWad& outWad) {

    // ── MANIFEST ──────────────────────────────────────────────────────────
    {
        ParsedEntry manifestFolder = MakeFolder(
            "Manifest", "GOWR_BLOCK_MANIFEST",
            WadEntryRole::ManifestBlock, WadBlock::Manifest);

        for (auto& e : m_entries) {
            if (e.block != WadBlock::Manifest || e.consumed) continue;
            if (e.role  == WadEntryRole::Sentinel)            continue; // omit PopHeap

            ParsedEntry node = ToNode(e, m_wadFilename);
            if (e.role == WadEntryRole::SharedWadRef)
                node.displayName = MakeSharedWadName(e.name);

            manifestFolder.children.push_back(std::move(node));
        }

        if (!manifestFolder.children.empty())
            outWad.entries.push_back(std::move(manifestFolder));
    }

    // ── SHADERS ───────────────────────────────────────────────────────────
    {
        ParsedEntry shadersFolder = MakeFolder(
            "Shaders", "GOWR_BLOCK_SHADERS",
            WadEntryRole::ShaderBlock, WadBlock::Shaders);

        ParsedEntry vsFolder = MakeFolder(
            "[Vertex Shaders]", "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
        ParsedEntry psFolder = MakeFolder(
            "[Pixel Shaders]", "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
        ParsedEntry hsFolder = MakeFolder(
            "[Hull Shaders]", "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
        ParsedEntry dsFolder = MakeFolder(
            "[Domain Shaders]", "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
        ParsedEntry csFolder = MakeFolder(
            "[Compute Shaders]", "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
        ParsedEntry lsFolder = MakeFolder(
            "[Library Shaders]", "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
        ParsedEntry containerFolder = MakeFolder(
            "[Containers]", "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);

        // Group VS by prefix before _vs_; PS by prefix before _ps_, etc
        std::map<std::string, ParsedEntry> vsGroups;
        std::map<std::string, ParsedEntry> psGroups;
        std::map<std::string, ParsedEntry> hsGroups;
        std::map<std::string, ParsedEntry> dsGroups;
        std::map<std::string, ParsedEntry> csGroups;
        std::map<std::string, ParsedEntry> lsGroups;

        for (auto& e : m_entries) {
            if (e.block != WadBlock::Shaders || e.consumed) continue;
            if (e.role  == WadEntryRole::Sentinel)           continue;

            if (e.role == WadEntryRole::ShaderVertex) {
                auto pos    = e.name.find("_vs_");
                std::string prefix = (pos != std::string::npos)
                                   ? e.name.substr(0, pos)
                                   : e.name;
                if (vsGroups.find(prefix) == vsGroups.end())
                    vsGroups[prefix] = MakeFolder(prefix, "GOWR_SHADER_GROUP",
                                                  WadEntryRole::ShaderGroup);
                vsGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderPixel) {
                auto pos    = e.name.find("_ps_");
                std::string prefix = (pos != std::string::npos)
                                   ? e.name.substr(0, pos)
                                   : e.name;
                if (psGroups.find(prefix) == psGroups.end())
                    psGroups[prefix] = MakeFolder(prefix, "GOWR_SHADER_GROUP",
                                                  WadEntryRole::ShaderGroup);
                psGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderHull) {
                auto pos    = e.name.find("_hs_");
                std::string prefix = (pos != std::string::npos) ? e.name.substr(0, pos) : e.name;
                if (hsGroups.find(prefix) == hsGroups.end())
                    hsGroups[prefix] = MakeFolder(prefix, "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
                hsGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderDomain) {
                auto pos    = e.name.find("_ds_");
                std::string prefix = (pos != std::string::npos) ? e.name.substr(0, pos) : e.name;
                if (dsGroups.find(prefix) == dsGroups.end())
                    dsGroups[prefix] = MakeFolder(prefix, "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
                dsGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderCompute) {
                auto pos    = e.name.find("_cs_");
                std::string prefix = (pos != std::string::npos) ? e.name.substr(0, pos) : e.name;
                if (csGroups.find(prefix) == csGroups.end())
                    csGroups[prefix] = MakeFolder(prefix, "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
                csGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderLibrary) {
                auto pos    = e.name.find("_ls_");
                std::string prefix = (pos != std::string::npos) ? e.name.substr(0, pos) : e.name;
                if (lsGroups.find(prefix) == lsGroups.end())
                    lsGroups[prefix] = MakeFolder(prefix, "GOWR_SHADER_GROUP", WadEntryRole::ShaderGroup);
                lsGroups[prefix].children.push_back(ToNode(e, m_wadFilename));

            } else if (e.role == WadEntryRole::ShaderContainer) {
                containerFolder.children.push_back(ToNode(e, m_wadFilename));
            }
        }

        // Flatten single-variant shader groups directly into the parent folder
        auto flushGroups = [](std::map<std::string, ParsedEntry>& groups,
                              ParsedEntry& targetFolder)
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
        ParsedEntry assetsFolder = MakeFolder(
            "Assets", "GOWR_BLOCK_ASSETS",
            WadEntryRole::AssetBlock, WadBlock::Assets);

        ParsedEntry lodFolder = MakeFolder(
            "[LOD Bindings]", "GOWR_LOD_BINDING_TABLE",
            WadEntryRole::LodBinding, WadBlock::Assets);

        for (auto& e : m_entries) {
            if (e.block != WadBlock::Assets || e.consumed) continue;

            if (e.role == WadEntryRole::TextureGpu) {
                // TexturePair flat node — GPU + CPU sub-entries are internal
                // streaming plumbing with no standalone view, so we hide them.
                ParsedEntry pairNode = ToNode(e, m_wadFilename);
                pairNode.role        = WadEntryRole::TexturePair;
                pairNode.schemaType  = "GOWR_TEXTURE_PAIR";
                pairNode.displayName = StripTextureHash(e.name);
                assetsFolder.children.push_back(std::move(pairNode));

            } else if (e.role == WadEntryRole::LodBinding) {
                lodFolder.children.push_back(ToNode(e, m_wadFilename));

            } else {
                ParsedEntry node = ToNode(e, m_wadFilename);
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
        ParsedEntry particlesFolder = MakeFolder(
            "Particles", "GOWR_BLOCK_PARTICLES",
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

                ParsedEntry fxGroup = MakeFolder(
                    ctx.empty() ? ("FX_" + e.name) : ctx,
                    "GOWR_FX_GROUP",
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

void WadNodeBuilder::Pass4_Finalize(OpenWad& outWad) {
    for (auto& blockNode : outWad.entries) {

        // ── Manifest: already in order; no sort needed ─────────────────
        if (blockNode.role == WadEntryRole::ManifestBlock)
            continue;

        // ── Shaders: sort groups alphabetically ─────────────────────
        if (blockNode.role == WadEntryRole::ShaderBlock) {
            for (auto& subFolder : blockNode.children) {
                if (subFolder.name == "[Vertex Shaders]" ||
                    subFolder.name == "[Pixel Shaders]" ||
                    subFolder.name == "[Hull Shaders]" ||
                    subFolder.name == "[Domain Shaders]" ||
                    subFolder.name == "[Compute Shaders]" ||
                    subFolder.name == "[Library Shaders]")
                {
                    std::sort(subFolder.children.begin(), subFolder.children.end(),
                        [](const ParsedEntry& a, const ParsedEntry& b) {
                            return a.name < b.name;
                        });
                    // Sort variants within each group by FLAGS suffix
                    for (auto& shaderGroup : subFolder.children) {
                        if (!shaderGroup.children.empty()) {
                            std::sort(shaderGroup.children.begin(), shaderGroup.children.end(),
                                [](const ParsedEntry& a, const ParsedEntry& b) {
                                    return a.name < b.name;
                                });
                        }
                    }
                }
            }
            continue;
        }

        // ── Assets: textures → materials → mesh/model → gameobj → audio ─
        if (blockNode.role == WadEntryRole::AssetBlock) {
            std::stable_sort(blockNode.children.begin(), blockNode.children.end(),
                [](const ParsedEntry& a, const ParsedEntry& b) {
                    int ka = AssetSortKey(a.role);
                    int kb = AssetSortKey(b.role);
                    if (ka != kb) return ka < kb;
                    // Within same category: sort by display name (falling back to name)
                    const std::string& na = a.displayName.empty() ? a.name : a.displayName;
                    const std::string& nb = b.displayName.empty() ? b.name : b.displayName;
                    return na < nb;
                });
            continue;
        }

        // ── Particles: FX groups sorted alphabetically ─────────────────
        if (blockNode.role == WadEntryRole::ParticleBlock) {
            std::sort(blockNode.children.begin(), blockNode.children.end(),
                [](const ParsedEntry& a, const ParsedEntry& b) {
                    // Folders before singletons
                    bool aFolder = !a.children.empty();
                    bool bFolder = !b.children.empty();
                    if (aFolder != bFolder) return aFolder > bFolder;
                    return a.name < b.name;
                });
            // Within each FxGroup: emitters → systems → material refs → protos → insts
            for (auto& fxGroup : blockNode.children) {
                if (fxGroup.role == WadEntryRole::FxGroup) {
                    std::stable_sort(fxGroup.children.begin(), fxGroup.children.end(),
                        [](const ParsedEntry& a, const ParsedEntry& b) {
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
                            return fxKey(a.role) < fxKey(b.role);
                        });
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

static TypeId RoleToTypeId(WadEntryRole role) {
    switch (role) {
        case WadEntryRole::ShaderContainer: return TypeId::ShaderContainer;
        case WadEntryRole::ShaderVertex: return TypeId::ShaderVertex;
        case WadEntryRole::ShaderPixel: return TypeId::ShaderPixel;
        case WadEntryRole::ShaderHull: return TypeId::ShaderHull;
        case WadEntryRole::ShaderDomain: return TypeId::ShaderDomain;
        case WadEntryRole::ShaderCompute: return TypeId::ShaderCompute;
        case WadEntryRole::ShaderLibrary: return TypeId::ShaderLibrary;
        case WadEntryRole::MeshGpu: return TypeId::MeshGpu;
        case WadEntryRole::MeshDefn: return TypeId::MeshDefn;
        case WadEntryRole::GameObjectProto: return TypeId::GameObjectProto;
        case WadEntryRole::GameObjectInst: return TypeId::GameObjectInst;
        case WadEntryRole::GameObjectOverride: return TypeId::GameObjectOverride;
        case WadEntryRole::TexturePair: return TypeId::TexturePair;
        case WadEntryRole::TextureGpu: return TypeId::TexturePair;
        case WadEntryRole::TextureCpu: return TypeId::TexturePair;
        case WadEntryRole::Material: return TypeId::Material;
        case WadEntryRole::MaterialRef: return TypeId::MaterialRef;
        case WadEntryRole::LodBinding: return TypeId::LodBinding;
        case WadEntryRole::AnimClip: return TypeId::AnimClip;
        case WadEntryRole::SoundEmitter: return TypeId::SoundEmitter;
        case WadEntryRole::ParticleEmitter: return TypeId::ParticleEmitter;
        case WadEntryRole::ParticleSystem: return TypeId::ParticleSystem;
        case WadEntryRole::ClientGuid: return TypeId::ClientGuid;
        case WadEntryRole::WadIdentity: return TypeId::WadIdentity;
        case WadEntryRole::SharedWadRef: return TypeId::SharedWadRef;
        case WadEntryRole::Sentinel: return TypeId::Sentinel;
        default: return TypeId::Unknown;
    }
}

ParsedEntry WadNodeBuilder::ToNode(const RawEntry& r, const std::string& wadFilename) {
    ParsedEntry e;
    e.name        = r.name;
    e.size        = r.size;
    e.offset      = r.offset;
    e.schemaType  = r.schemaType;
    e.wadName     = wadFilename;
    e.role        = r.role;
    e.block       = r.block;
    e.typeId      = RoleToTypeId(r.role);
    e.displayName = r.displayName;
    return e;
}

ParsedEntry WadNodeBuilder::MakeFolder(
    const std::string& name,
    const std::string& schemaType,
    WadEntryRole       role,
    WadBlock           block) const
{
    ParsedEntry f;
    f.name       = name;
    f.schemaType = schemaType;
    f.role       = role;
    f.block      = block;
    f.wadName    = m_wadFilename;
    f.size       = 0;
    f.offset     = 0;
    f.typeId     = RoleToTypeId(role);
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

} // namespace GOW
