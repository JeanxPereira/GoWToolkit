#include "core/loaders/GOWRLoaders.h"
#include "core/types/TypeRegistry.h"
#include "core/WadTypes.h"
#include "core/schema/AssetReader.h"
#include "core/formats/GOWRMeshDefnFormat.h"
#include "core/parsers/gowr/MeshParser.h"
#include "core/parsers/gowr/LodPackIndex.h"
#include "core/parsers/gowr/TexPackIndex.h"
#include "core/parsers/gowr/ProtoParser.h"
#include "core/parsers/gowr/MgParser.h"
#include "core/parsers/gowr/ShaderParser.h"
#include "core/parsers/shared/SceneNode.h"
#include "ui/viewers/ImageViewer.h"
#include "core/Logger.h"
#include "core/vfs/SliceFile.h"
#include "core/vfs/MemoryFile.h"
#include "ui/viewers/Viewport3D.h"
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>
#include <fstream>
#include "../parsers/gowr/Rdna2Detiler.h"
#include "../parsers/gowr/BcDecoder.h"

using GOW::Rdna2::Detile;

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

// ── GOWRLoaders.cpp ────────────────────────────────────────────────────────

namespace GOW {

// ── Search candidates for runtime files (config.ini, lodpacks.txt) ─────────
static std::vector<std::filesystem::path> ResourceSearchDirs() {
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(std::filesystem::current_path());

#ifdef __APPLE__
    char exeBuf[4096] = {};
    uint32_t bufSize = sizeof(exeBuf);
    if (_NSGetExecutablePath(exeBuf, &bufSize) == 0) {
        try {
            auto exePath = std::filesystem::path(exeBuf);
            auto exeDir  = exePath.parent_path();
            candidates.push_back(exeDir);
            // Walk up: MacOS -> Contents -> .app -> build/
            candidates.push_back(exeDir.parent_path().parent_path().parent_path());
        } catch (...) {}
    }
#endif
    return candidates;
}

static std::filesystem::path FindResource(const std::string& filename) {
    for (const auto& dir : ResourceSearchDirs()) {
        auto p = dir / filename;
        if (std::filesystem::exists(p)) return p;
    }
    return {};
}

// ── Resolve game root from config.ini ─────────────────────────────────────
static std::filesystem::path ReadGameRootFromConfig() {
    auto configPath = FindResource("config.ini");
    if (configPath.empty()) return {};
    LOG_INFO("[GOWRLoaders] Found config.ini at: %s", configPath.string().c_str());

    std::ifstream cfg(configPath);
    if (!cfg.is_open()) return {};

    std::string line;
    std::getline(cfg, line); // skip first line (comment)
    if (!std::getline(cfg, line)) return {};

    // Expect "gameroot=D:\..." or just the path on line 1
    auto eq = line.find('=');
    if (eq != std::string::npos)
        line = line.substr(eq + 1);

    // Trim whitespace and quotes
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '"'))
        line.pop_back();
    while (!line.empty() && (line.front() == ' ' || line.front() == '"'))
        line.erase(0, 1);

    return std::filesystem::path(line);
}

// ── LodPackIndex singleton ─────────────────────────────────────────────────

static LodPackIndex* s_lodIndex = nullptr;

static LodPackIndex& GetLodIndex() {
    if (!s_lodIndex) {
        s_lodIndex = new LodPackIndex();
        auto lodpacksTxt = FindResource("lodpacks.txt");
        auto gameRoot    = ReadGameRootFromConfig();

        if (!gameRoot.empty() && !lodpacksTxt.empty()) {
            LOG_INFO("[GOWRLoaders] Found lodpacks.txt at: %s", lodpacksTxt.string().c_str());
            s_lodIndex->LoadFromList(lodpacksTxt, gameRoot);
        } else {
            LOG_WARN("[GOWRLoaders] config.ini or lodpacks.txt not found — LOD lookup disabled");
        }
    }
    return *s_lodIndex;
}

static TexPackIndex* s_texIndex = nullptr;
static std::mutex s_texIndexMutex;
static bool s_texIndexStarted = false;

static void StartTexIndexLoad() {
    auto gameRoot = ReadGameRootFromConfig();
    if (!gameRoot.empty()) {
        s_texIndex->LoadFromGameRoot(gameRoot);
    } else {
        LOG_WARN("[GOWRLoaders] config.ini not found — Texture lookup from texpack disabled");
        s_texIndex->SetLoaded();
    }
}

TexPackIndex& GetTexIndex() {
    std::lock_guard<std::mutex> lock(s_texIndexMutex);
    if (!s_texIndex) {
        s_texIndex = new TexPackIndex();
    }
    if (!s_texIndexStarted) {
        s_texIndexStarted = true;
        // Launch indexing on a background thread — does NOT block UI
        std::thread(StartTexIndexLoad).detach();
    }
    return *s_texIndex;
}

void InvalidateLodIndex() {
    delete s_lodIndex;
    s_lodIndex = nullptr;
    delete s_texIndex;
    s_texIndex = nullptr;
}

// ── GOWR Mesh Handling ─────────────────────────────────────────────────────

static std::shared_ptr<IDocumentContent> SharedGowrMeshLoad(const ParsedEntry& entry, OpenWad& wad, bool attachSkeleton) {
    if (!wad.fileSource) return nullptr;

    // ── Slice the MESH file ────────────────────────────────────────────
    auto meshFile = std::make_shared<SliceFile>(
        wad.fileSource, entry.offset, entry.size);

    // ── Find the paired MG_GPU sibling ────────────────────────────────
    // Strip prefix (MESH_ or MG_) and trailing ---NNNNN hash to get the base name.
    std::string base = entry.name;
    for (const char* pfx : {"MESH_", "MG_"}) {
        if (base.rfind(pfx, 0) == 0) { base = base.substr(strlen(pfx)); break; }
    }
    auto dashPos = base.rfind("---");
    if (dashPos != std::string::npos) base = base.substr(0, dashPos);

    std::shared_ptr<IFile> gpuFile;

    std::function<const ParsedEntry*(const std::vector<ParsedEntry>&)> findGpu;
    findGpu = [&](const std::vector<ParsedEntry>& entries) -> const ParsedEntry* {
        for (const auto& e : entries) {
            if (e.role == WadEntryRole::MeshGpu) {
                std::string gpuBase = e.name;
                if (gpuBase.rfind("MG_", 0) == 0) gpuBase = gpuBase.substr(3);
                auto d = gpuBase.rfind("---");
                if (d != std::string::npos) gpuBase = gpuBase.substr(0, d);
                if (gpuBase.size() > 4 &&
                    gpuBase.substr(gpuBase.size() - 4) == "_gpu")
                    gpuBase = gpuBase.substr(0, gpuBase.size() - 4);
                if (gpuBase == base) return &e;
            }
            if (!e.children.empty()) {
                auto* found = findGpu(e.children);
                if (found) return found;
            }
        }
        return nullptr;
    };

    const ParsedEntry* gpuEntry = findGpu(wad.entries);
    if (gpuEntry) {
        gpuFile = std::make_shared<SliceFile>(
            wad.fileSource, gpuEntry->offset, gpuEntry->size);
        LOG_INFO("[GOWRLoaders] GPU: %s (size=%u)",
                 gpuEntry->name.c_str(), gpuEntry->size);
    } else {
        LOG_WARN("[GOWRLoaders] No MeshGpu sibling for '%s' — hash=0 submeshes only",
                 entry.name.c_str());
    }

    // ── Parse ──────────────────────────────────────────────────────────
    MeshData data;
    bool ok = false;

    const LodPackIndex& lodIdx = GetLodIndex();
    if (gpuFile && lodIdx.TotalEntries() > 0) {
        ok = GOWRMeshParser::ParseWithLodPack(meshFile, gpuFile, lodIdx, data);
    } else if (gpuFile) {
        ok = GOWRMeshParser::Parse(meshFile, gpuFile, data);
    }

    if (!ok || data.parts.empty()) {
        LOG_WARN("[GOWRLoaders] Parse failed or no parts for '%s'", entry.name.c_str());
        return std::make_shared<Viewport3D>(entry.name);
    }

    // ── Find paired MG_<base> file (bone-binding, no _gpu suffix) ─────
    std::shared_ptr<IFile> mgFile;
    std::function<const ParsedEntry*(const std::vector<ParsedEntry>&)> findMg;
    findMg = [&](const std::vector<ParsedEntry>& entries) -> const ParsedEntry* {
        for (const auto& e : entries) {
            if (e.role == WadEntryRole::MeshDefn &&
                e.name.rfind("MG_", 0) == 0)
            {
                std::string n = e.name.substr(3); // strip "MG_"
                if (n.size() > 4 && n.substr(n.size() - 4) == "_gpu") continue;
                auto d = n.rfind("---");
                if (d != std::string::npos) n = n.substr(0, d);
                if (n == base) return &e;
            }
            if (!e.children.empty()) {
                auto* f = findMg(e.children);
                if (f) return f;
            }
        }
        return nullptr;
    };

    const ParsedEntry* mgEntry = findMg(wad.entries);
    if (mgEntry) {
        mgFile = std::make_shared<SliceFile>(
            wad.fileSource, mgEntry->offset, mgEntry->size);
        LOG_INFO("[GOWRLoaders] MG bone-binding: %s (size=%u)",
                 mgEntry->name.c_str(), mgEntry->size);
    }

    // ── DIAGNOSTIC: locate paired MDL_<base> and dump first 512 bytes plus
    // a tail sample. Hunting for MAT hash field placement. Remove once link
    // identified.
    {
        std::string mdlBase = base;
        // Strip trailing "_<digits>" suffix (e.g. "athena10_0" → "athena10")
        auto usPos = mdlBase.find_last_of('_');
        if (usPos != std::string::npos && usPos + 1 < mdlBase.size()) {
            bool allDigits = true;
            for (size_t k = usPos + 1; k < mdlBase.size(); ++k) {
                if (!isdigit((unsigned char)mdlBase[k])) { allDigits = false; break; }
            }
            if (allDigits) mdlBase = mdlBase.substr(0, usPos);
        }

        std::function<const ParsedEntry*(const std::vector<ParsedEntry>&)> findMdl;
        findMdl = [&](const std::vector<ParsedEntry>& entries) -> const ParsedEntry* {
            for (const auto& e : entries) {
                if (e.role == WadEntryRole::Model && e.name.rfind("MDL_", 0) == 0) {
                    std::string n = e.name.substr(4);
                    auto d = n.rfind("---");
                    if (d != std::string::npos) n = n.substr(0, d);
                    if (n == mdlBase) return &e;
                }
                if (!e.children.empty()) {
                    auto* f = findMdl(e.children);
                    if (f) return f;
                }
            }
            return nullptr;
        };

        if (const ParsedEntry* mdlEntry = findMdl(wad.entries)) {
            const uint32_t dumpSz = std::min<uint32_t>(mdlEntry->size, 512u);
            std::vector<uint8_t> buf(dumpSz);
            wad.fileSource->Seek(mdlEntry->offset, 0);
            wad.fileSource->Read(buf.data(), dumpSz);
            std::string hex; hex.reserve(dumpSz * 3 + 8);
            char tmp[4];
            for (uint32_t b = 0; b < dumpSz; ++b) {
                std::snprintf(tmp, sizeof(tmp), "%02X ", buf[b]);
                hex += tmp;
            }
            LOG_INFO("[GOWRLoaders] MDL '%s' size=%u first %u bytes: %s",
                     mdlEntry->name.c_str(), mdlEntry->size, dumpSz, hex.c_str());

            // Also dump last 256 bytes if file is larger than dumpSz
            if (mdlEntry->size > 512) {
                const uint32_t tailSz = std::min<uint32_t>(mdlEntry->size - 512, 256u);
                std::vector<uint8_t> tail(tailSz);
                wad.fileSource->Seek(mdlEntry->offset + mdlEntry->size - tailSz, 0);
                wad.fileSource->Read(tail.data(), tailSz);
                std::string thex; thex.reserve(tailSz * 3 + 8);
                for (uint32_t b = 0; b < tailSz; ++b) {
                    std::snprintf(tmp, sizeof(tmp), "%02X ", tail[b]);
                    thex += tmp;
                }
                LOG_INFO("[GOWRLoaders] MDL tail %u bytes: %s", tailSz, thex.c_str());
            }
        } else {
            LOG_INFO("[GOWRLoaders] No MDL_ sibling for '%s' (base='%s')",
                     entry.name.c_str(), mdlBase.c_str());
        }
    }

    // ── Find paired goProto* file (skeleton) ───────────────────────────
    // Skip entirely when the caller asked for a mesh-only view (MESH_ entry).
    // Rigged viewers (goProto*, go*) pass attachSkeleton=true and resolve the
    // rig via name pairing below.
    std::shared_ptr<ObjectData> skeleton;
    if (attachSkeleton) {
        // Naming: MESH_athena10_0 → look for goProtoathena10 (strip MESH_/MG_,
        // trailing _N, and trailing ---HASH).
        std::string protoBase = base;
        // Strip trailing "_<digits>" suffix if present (e.g. "athena10_0" → "athena10")
        auto usPos = protoBase.find_last_of('_');
        if (usPos != std::string::npos && usPos + 1 < protoBase.size()) {
            bool allDigits = true;
            for (size_t k = usPos + 1; k < protoBase.size(); ++k) {
                if (!isdigit((unsigned char)protoBase[k])) { allDigits = false; break; }
            }
            if (allDigits) protoBase = protoBase.substr(0, usPos);
        }

        std::function<const ParsedEntry*(const std::vector<ParsedEntry>&)> findProto;
        findProto = [&](const std::vector<ParsedEntry>& entries) -> const ParsedEntry* {
            for (const auto& e : entries) {
                if (e.role == WadEntryRole::GameObjectProto) {
                    std::string n = e.name;
                    if (n.rfind("goProto", 0) == 0) n = n.substr(7);
                    auto d = n.rfind("---");
                    if (d != std::string::npos) n = n.substr(0, d);
                    if (n == protoBase) return &e;
                }
                if (!e.children.empty()) {
                    auto* f = findProto(e.children);
                    if (f) return f;
                }
            }
            return nullptr;
        };

        const ParsedEntry* protoEntry = findProto(wad.entries);
        if (protoEntry) {
            auto protoFile = std::make_shared<SliceFile>(
                wad.fileSource, protoEntry->offset, protoEntry->size);
            skeleton = GOWRProtoParser::Parse(protoFile);
            if (skeleton) {
                LOG_INFO("[GOWRLoaders] Proto rig '%s': %zu bones",
                         protoEntry->name.c_str(), skeleton->joints.size());
            }
        } else {
            LOG_INFO("[GOWRLoaders] No goProto sibling for '%s' (base='%s')",
                     entry.name.c_str(), protoBase.c_str());
        }
    }

    // ── Build SceneData and load into viewport ─────────────────────────
    auto vp = std::make_shared<Viewport3D>(entry.name);

    if (skeleton) {
        // ── Skinning resolution (CURRENT: rigid-only fallback) ────────────
        // GOWR skinned vertex bone-indices are per-submesh LOCAL palette
        // indices (not global). The palette source — MG file vs MESH submesh
        // header vs separate VPK — is still unknown. Treating them as global
        // (identity jointMap) causes catastrophic vertex explosions because
        // small palette indices map to arbitrary global bones.
        //
        // Until the palette is located, force every part to rigid: bind all
        // verts to the MG parentBone with weight 1. This loses skinning but
        // keeps the mesh shape stable.
        // TODO: locate per-submesh bone palette (MG skinList? MESH header?
        //       VPK chunk?) and wire jointMap[localIdx] = globalIdx.
        if (mgFile) {
            uint32_t meshSubCount = 0;
            for (const auto& p : data.parts)
                if (p.materialId + 1 > meshSubCount) meshSubCount = p.materialId + 1;

            std::vector<uint16_t> parentBone;
            if (GOWRMgParser::Parse(mgFile, meshSubCount, parentBone)) {
                for (auto& p : data.parts) {
                    uint16_t pb = (p.materialId < parentBone.size())
                                    ? parentBone[p.materialId] : 0xFFFF;
                    if (pb == 0xFFFF) pb = 0;
                    p.jointMap = { pb };
                    for (auto& v : p.vertices) {
                        v.boneIndices = glm::uvec4(0, 0, 0, 0);
                        v.boneWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                    }
                }
            }
        }

        auto scene = std::make_unique<SceneData>();
        scene->skeleton  = skeleton;
        scene->flipZ     = true;    // mesh and bones both face -Z; flip once for screen
        scene->meshParts = std::move(data.parts);
        vp->LoadScene(std::move(scene));
    } else {
        std::vector<std::unique_ptr<TextureData>> noTextures;
        vp->LoadFromMeshData(data, noTextures);
    }
    return vp;
}

std::shared_ptr<AssetNode> GOWRMeshDefnHandler::Parse(std::shared_ptr<IFile> file) {
    if (!file || file->Size() < 64) return nullptr;
    GOWRMeshDefnFormat format;
    format.Initialize();
    return AssetReader::Parse(*format.Root(), file);
}

std::shared_ptr<IDocumentContent> GOWRMeshDefnHandler::CreateViewer(const ParsedEntry& entry, OpenWad& wad) {
    // MESH_* — render the 3D model without binding to a skeleton.
    return SharedGowrMeshLoad(entry, wad, /*attachSkeleton=*/false);
}
std::shared_ptr<IDocumentContent> GOWRSkinnedMeshHandler::CreateViewer(const ParsedEntry& entry, OpenWad& wad) {
    // GOWR_SKINNED_MESH — attach the rig so bones drive the mesh.
    return SharedGowrMeshLoad(entry, wad, /*attachSkeleton=*/true);
}
std::shared_ptr<IDocumentContent> GOWRModelInstanceHandler::CreateViewer(const ParsedEntry& entry, OpenWad& wad) {
    // go* instance — load with rig.
    return SharedGowrMeshLoad(entry, wad, /*attachSkeleton=*/true);
}

#include <imgui.h>
#include "ui/viewers/ImageViewer.h"

class GOWRTextureViewer : public IDocumentContent {
public:
    GOWRTextureViewer(const ParsedEntry& entry) : m_entry(entry), m_name(entry.name) {
        auto lastUs = m_name.find_last_of('_');
        if (lastUs != std::string::npos && lastUs + 1 < m_name.size()) {
            try { m_hash = std::stoull(m_name.substr(lastUs + 1), nullptr, 16); } catch(...) {}
        }
    }

    std::string GetName() const override { return "Tex: " + m_name; }

    void Draw() override {
        auto& texIdx = GetTexIndex();

        if (!texIdx.IsLoaded()) {
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x * 0.5f - 150, ImGui::GetWindowSize().y * 0.5f - 20));
            ImGui::Text("Loading Texture Index (TOCs) in background...");
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.5f - 150);
            ImGui::ProgressBar(texIdx.GetLoadProgress(), ImVec2(300, 0));
            return;
        }

        if (!m_initialized) {
            m_initialized = true;
            FirstLoad(texIdx);
        }

        if (m_failReason) {
            ImGui::TextColored(ImVec4(1,0.6f,0.6f,1), "Texture load failed: %s", m_failReason);
            ImGui::Text("Name: %s", m_name.c_str());
            ImGui::Text("Hash: 0x%016llx", (unsigned long long)m_hash);
            if (m_texW || m_texH)   ImGui::Text("Size: %ux%u", m_texW, m_texH);
            if (m_swMode != 0xFFFF) ImGui::Text("sw_mode: %u  pipeBankXor: 0x%x", m_swMode, m_pipeBankXor);
            return;
        }

        if (m_realViewer) m_realViewer->Draw();
    }

private:
    ParsedEntry m_entry;
    std::string m_name;
    uint64_t    m_hash = 0;
    bool        m_initialized = false;
    const char* m_failReason  = nullptr;

    std::shared_ptr<ImageViewer> m_realViewer;

    uint32_t  m_texW = 0, m_texH = 0;
    uint32_t  m_swMode = 0xFFFF;
    uint32_t  m_pipeBankXor = 0;
    BcFormat  m_bcFmt = BcFormat::BC1;

    void FirstLoad(TexPackIndex& texIdx) {
        if (!m_hash) { m_failReason = "no hash in name"; return; }

        TexpackEntry texEntry;
        if (!texIdx.FindTexture(m_hash, texEntry)) { m_failReason = "hash not in texpack index"; return; }

        auto file = texIdx.GetFile(texEntry.packIdx);
        if (!file || !file->IsValid()) { m_failReason = "texpack file unavailable"; return; }

        m_texW = texEntry.width;
        m_texH = texEntry.height;

        // Block layout: 16B header + 256B GNF descriptor + tiled data at +0x124.
        file->Seek(static_cast<int64_t>(texEntry.blockDataOffset), 0);
        uint32_t bMagic = 0, bDataOff = 0, bLen = 0, bUnk = 0;
        file->Read(&bMagic, 4); file->Read(&bDataOff, 4); file->Read(&bLen, 4); file->Read(&bUnk, 4);

        uint8_t gnfBuf[0x100] = {};
        file->Read(gnfBuf, 0x100);

        // PS5 AGC T# at +0x10 (after 16-byte GNF header). 8 dwords.
        // dw3 bits[24:20] = sw_mode, bits[21:8] = pipeBankXor (14-bit).
        // dw1 bits[25:20] = data_format (AGC enum — empirically BC1=0x2A here).
        const uint32_t dw1 = *reinterpret_cast<uint32_t*>(gnfBuf + 0x14);
        const uint32_t dw3 = *reinterpret_cast<uint32_t*>(gnfBuf + 0x1C);

        m_swMode      = (dw3 >> 20) & 0x1F;
        m_pipeBankXor = (dw3 >> 8)  & 0x3FFF;
        const uint32_t dataFmt = (dw1 >> 20) & 0x3F;

        // Map AGC data_format → BC format. Values verified against DDS FourCC
        // (DXT1/ATI1/ATI2/DX10) for GOWR PC.
        bool isBc = true;
        switch (dataFmt) {
            case 0x29: m_bcFmt = BcFormat::BC1; break;  // BC1 SRGB
            case 0x2A: m_bcFmt = BcFormat::BC1; break;  // BC1 UNORM
            case 0x2F: m_bcFmt = BcFormat::BC4; break;  // BC4 (ATI1)
            case 0x31: m_bcFmt = BcFormat::BC5; break;  // BC5 (ATI2)
            case 0x35: m_bcFmt = BcFormat::BC7; break;  // BC7 SRGB
            case 0x36: m_bcFmt = BcFormat::BC7; break;  // BC7 UNORM
            default:
                isBc = false;
                break;
        }
        if (!isBc) { m_failReason = "unsupported AGC data_format"; return; }

        const uint32_t blockBytes = BcBlockSize(m_bcFmt);
        const uint32_t blocksX = (m_texW + 3) / 4;
        const uint32_t blocksY = (m_texH + 3) / 4;
        const size_t   linearBcSz = size_t(blocksX) * blocksY * blockBytes;

        // Multi-mip blocks store mips smallest→largest; mip0 occupies the last
        // `linearBcSz` bytes of the block's raw data area.
        const int64_t mip0Off = static_cast<int64_t>(texEntry.blockDataOffset)
                              + static_cast<int64_t>(bDataOff)
                              + static_cast<int64_t>(texEntry.rawSize)
                              - static_cast<int64_t>(linearBcSz);
        std::vector<uint8_t> tiled(linearBcSz);
        file->Seek(mip0Off, 0);
        file->Read(tiled.data(), tiled.size());

        std::vector<uint8_t> linearBc(linearBcSz, 0);
        bool detiled = false;
        if (m_swMode == 0) {
            std::memcpy(linearBc.data(), tiled.data(), linearBcSz);
            detiled = true;
        } else {
            detiled = Rdna2::Detile(tiled.data(), tiled.size(),
                                    linearBc.data(),
                                    blocksX, blocksY, blockBytes,
                                    m_swMode, m_pipeBankXor);
        }
        if (!detiled) { m_failReason = "no detile equation for this sw_mode"; return; }

        std::vector<uint8_t> rgba;
        if (!DecompressBc(linearBc.data(), linearBcSz, m_texW, m_texH, m_bcFmt, rgba)) {
            m_failReason = "BC decompress failed";
            return;
        }

        auto texData = std::make_unique<TextureData>();
        texData->name = m_name;
        texData->width = m_texW;
        texData->height = m_texH;
        texData->isCompressed = false;
        texData->glInternalFormat = 0x1908;  // GL_RGBA
        texData->dataSize = rgba.size();
        texData->pixels = std::move(rgba);

        m_realViewer = std::make_shared<ImageViewer>(m_name, std::move(texData));
    }
};

std::shared_ptr<IDocumentContent> GOWRTextureHandler::CreateViewer(const ParsedEntry& entry, OpenWad& wad) {
    return std::make_shared<GOWRTextureViewer>(entry);
}

std::shared_ptr<IDocumentContent> GOWRRigHandler::CreateViewer(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;

    // Derive the base name: "goProtofox00" → "fox00"
    std::string protoBase = entry.name;
    if (protoBase.rfind("goProto", 0) == 0) protoBase = protoBase.substr(7);
    auto dashPos = protoBase.rfind("---");
    if (dashPos != std::string::npos) protoBase = protoBase.substr(0, dashPos);

    // Find the first MESH_<base>* entry in the WAD
    std::function<const ParsedEntry*(const std::vector<ParsedEntry>&)> findMesh;
    findMesh = [&](const std::vector<ParsedEntry>& entries) -> const ParsedEntry* {
        for (const auto& e : entries) {
            if (e.role == WadEntryRole::MeshDefn &&
                e.name.rfind("MESH_", 0) == 0)
            {
                std::string n = e.name.substr(5); // strip "MESH_"
                auto d = n.rfind("---");
                if (d != std::string::npos) n = n.substr(0, d);
                // Strip trailing "_<digits>" LOD suffix
                auto us = n.find_last_of('_');
                if (us != std::string::npos && us + 1 < n.size()) {
                    bool allDigits = true;
                    for (size_t k = us + 1; k < n.size(); ++k)
                        if (!isdigit((unsigned char)n[k])) { allDigits = false; break; }
                    if (allDigits) n = n.substr(0, us);
                }
                if (n == protoBase) return &e;
            }
            if (!e.children.empty()) {
                auto* f = findMesh(e.children);
                if (f) return f;
            }
        }
        return nullptr;
    };

    const ParsedEntry* meshEntry = findMesh(wad.entries);
    if (meshEntry) {
        LOG_INFO("[GOWRRigHandler] Found MESH '%s' for proto '%s'",
                 meshEntry->name.c_str(), entry.name.c_str());
        return SharedGowrMeshLoad(*meshEntry, wad, /*attachSkeleton=*/true);
    }

    // Fallback: try MG_<base>* (non-gpu) entries
    std::function<const ParsedEntry*(const std::vector<ParsedEntry>&)> findMg;
    findMg = [&](const std::vector<ParsedEntry>& entries) -> const ParsedEntry* {
        for (const auto& e : entries) {
            if (e.role == WadEntryRole::MeshDefn &&
                e.name.rfind("MG_", 0) == 0)
            {
                std::string n = e.name.substr(3);
                if (n.size() > 4 && n.substr(n.size() - 4) == "_gpu") {
                    if (!e.children.empty()) { auto* f = findMg(e.children); if (f) return f; }
                    continue;
                }
                auto d = n.rfind("---");
                if (d != std::string::npos) n = n.substr(0, d);
                auto us = n.find_last_of('_');
                if (us != std::string::npos && us + 1 < n.size()) {
                    bool allDigits = true;
                    for (size_t k = us + 1; k < n.size(); ++k)
                        if (!isdigit((unsigned char)n[k])) { allDigits = false; break; }
                    if (allDigits) n = n.substr(0, us);
                }
                if (n == protoBase) return &e;
            }
            if (!e.children.empty()) {
                auto* f = findMg(e.children);
                if (f) return f;
            }
        }
        return nullptr;
    };

    const ParsedEntry* mgEntry = findMg(wad.entries);
    if (mgEntry) {
        LOG_INFO("[GOWRRigHandler] Found MG '%s' for proto '%s' (fallback)",
                 mgEntry->name.c_str(), entry.name.c_str());
        return SharedGowrMeshLoad(*mgEntry, wad, /*attachSkeleton=*/true);
    }

    LOG_WARN("[GOWRRigHandler] No MESH/MG found for proto '%s' (base='%s')",
             entry.name.c_str(), protoBase.c_str());
    return nullptr;
}

REGISTER_FILE_TYPE(GOWRMeshDefnHandler);
REGISTER_FILE_TYPE(GOWRSkinnedMeshHandler);
REGISTER_FILE_TYPE(GOWRModelInstanceHandler);
REGISTER_FILE_TYPE(GOWRTextureHandler);
REGISTER_FILE_TYPE(GOWRRigHandler);

// ── GOWR Shader Viewer ────────────────────────────────────────────────────

class GOWRShaderViewer : public IDocumentContent {
public:
    GOWRShaderViewer(const std::string& name, std::unique_ptr<GOWRShaderData> data)
        : m_name(name), m_data(std::move(data)) {}

    std::string GetName() const override { return "Shader: " + m_name; }

    void Draw() override {
        if (!m_data) {
            ImGui::TextDisabled("Failed to parse shader");
            return;
        }

        // ── GOW Header ─────────────────────────────────────────────────
        ImGui::SeparatorText("GOW Shader Header");
        if (ImGui::BeginTable("##gowhdr", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field",  ImGuiTableColumnFlags_WidthFixed, 140);
            ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch);

            Row("Stage",    "%s (%s)", m_data->stageTag.c_str(), m_data->StageName());
            Row("Version",  "%u.%u",  m_data->formatVersion, m_data->subVersion);
            Row("DXBC Size","%u bytes (%.1f KB)", m_data->dxbcSize, m_data->dxbcSize / 1024.0f);
            Row("PSO Flags","0x%08X", m_data->psoFlags);
            Row("Variant",  "0x%08X", m_data->variantId);

            ImGui::EndTable();
        }

        if (!m_data->hasDxbc) {
            ImGui::TextColored(ImVec4(1,0.5f,0.5f,1), "No valid DXBC container found.");
            return;
        }

        // ── Debug path (ILDN) ──────────────────────────────────────────
        if (!m_data->debugPath.empty()) {
            ImGui::Spacing();
            ImGui::SeparatorText("Build Path (ILDN)");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
            ImGui::TextWrapped("%s", m_data->debugPath.c_str());
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                ImGui::SetClipboardText(m_data->debugPath.c_str());
        }

        // ── DXIL Info ──────────────────────────────────────────────────
        if (m_data->dxil.valid) {
            ImGui::Spacing();
            ImGui::SeparatorText("DXIL Payload");
            if (ImGui::BeginTable("##dxil", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Field",  ImGuiTableColumnFlags_WidthFixed, 140);
                ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch);
                Row("Shader Model", "%u.%u", m_data->dxil.majorVersion, m_data->dxil.minorVersion);
                Row("Bitcode Size", "%u bytes", m_data->dxil.bitcodeSize);
                ImGui::EndTable();
            }
        }

        // ── Chunks ─────────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::SeparatorText("DXBC Chunks");
        if (ImGui::BeginTable("##chunks", 3,
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("FourCC",  ImGuiTableColumnFlags_WidthFixed,  60);
            ImGui::TableSetupColumn("Offset",  ImGuiTableColumnFlags_WidthFixed,  80);
            ImGui::TableSetupColumn("Size",    ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableHeadersRow();

            for (const auto& c : m_data->chunks) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", c.tag);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("0x%04X", c.offset);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%u", c.size);
            }
            ImGui::EndTable();
        }

        // ── Input Signature ────────────────────────────────────────────
        DrawSignature("Input Signature (ISG1)", m_data->inputs);

        // ── Output Signature ───────────────────────────────────────────
        DrawSignature("Output Signature (OSG1)", m_data->outputs);

        // ── Patch Signature ────────────────────────────────────────────
        if (!m_data->patch.empty()) {
            DrawSignature("Patch Signature (PSG1)", m_data->patch);
        }

        // ── Statistics ─────────────────────────────────────────────────
        if (m_data->stats.valid) {
            ImGui::Spacing();
            ImGui::SeparatorText("Statistics (STAT)");
            if (ImGui::BeginTable("##stats", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 160);
                ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch);
                Row("Instructions",    "%u", m_data->stats.instructionCount);
                Row("Temp Registers",  "%u", m_data->stats.tempRegisterCount);
                Row("Float Ops",       "%u", m_data->stats.floatOps);
                Row("Int Ops",         "%u", m_data->stats.intOps);
                Row("UInt Ops",        "%u", m_data->stats.uintOps);
                Row("Texture Ops",     "%u", m_data->stats.textureOps);
                ImGui::EndTable();
            }
        }

        // ── Shader Hash ────────────────────────────────────────────────
        if (m_data->hasHash) {
            ImGui::Spacing();
            ImGui::SeparatorText("Shader Hash");
            char hashStr[41] = {};
            for (int i = 0; i < 16; i++)
                snprintf(hashStr + i * 2, 3, "%02x", m_data->shaderHash[i]);
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.5f, 1.0f), "%s", hashStr);
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                ImGui::SetClipboardText(hashStr);
        }
    }

private:
    std::string m_name;
    std::unique_ptr<GOWRShaderData> m_data;

    template<typename... Args>
    void Row(const char* key, const char* fmt, Args... args) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("%s", key);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text(fmt, args...);
    }

    void DrawSignature(const char* title, const std::vector<SignatureElement>& sig) {
        if (sig.empty()) return;
        ImGui::Spacing();
        ImGui::SeparatorText(title);
        if (ImGui::BeginTable("##sig", 5,
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableSetupColumn("Semantic",   ImGuiTableColumnFlags_WidthFixed, 180);
            ImGui::TableSetupColumn("Index",      ImGuiTableColumnFlags_WidthFixed,  50);
            ImGui::TableSetupColumn("Type",       ImGuiTableColumnFlags_WidthFixed,  60);
            ImGui::TableSetupColumn("Register",   ImGuiTableColumnFlags_WidthFixed,  60);
            ImGui::TableSetupColumn("Mask",       ImGuiTableColumnFlags_WidthFixed,  60);
            ImGui::TableHeadersRow();

            for (const auto& e : sig) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (e.systemValueType != 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "%s",
                                       GOWRShaderData::SystemValueName(e.systemValueType));
                } else {
                    ImGui::Text("%s", e.semanticName.c_str());
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", e.semanticIndex);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", GOWRShaderData::ComponentTypeName(e.componentType));
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("r%u", e.registerIndex);
                ImGui::TableSetColumnIndex(4);
                std::string mask = GOWRShaderData::MaskString(e.mask);
                ImGui::Text("%s", mask.c_str());
            }
            ImGui::EndTable();
        }
    }
};

std::shared_ptr<IDocumentContent> GOWRShaderHandler::CreateViewer(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource || entry.size == 0) return nullptr;

    auto file = std::make_shared<SliceFile>(wad.fileSource, entry.offset, entry.size);
    auto data = GOWRShaderParse(file);
    if (!data) return nullptr;

    return std::make_shared<GOWRShaderViewer>(entry.name, std::move(data));
}

// Register shader handlers for all shader TypeIds
static bool _reg_shader_vs = [] {
    ::GOW::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<GOW::GOWRShaderHandler>(GOW::TypeId::ShaderVertex));
    return true;
}();
static bool _reg_shader_ps = [] {
    ::GOW::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<GOW::GOWRShaderHandler>(GOW::TypeId::ShaderPixel));
    return true;
}();
static bool _reg_shader_ct = [] {
    ::GOW::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<GOW::GOWRShaderHandler>(GOW::TypeId::ShaderContainer));
    return true;
}();
static bool _reg_shader_hs = [] {
    ::GOW::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<GOW::GOWRShaderHandler>(GOW::TypeId::ShaderHull));
    return true;
}();
static bool _reg_shader_ds = [] {
    ::GOW::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<GOW::GOWRShaderHandler>(GOW::TypeId::ShaderDomain));
    return true;
}();
static bool _reg_shader_cs = [] {
    ::GOW::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<GOW::GOWRShaderHandler>(GOW::TypeId::ShaderCompute));
    return true;
}();
static bool _reg_shader_ls = [] {
    ::GOW::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<GOW::GOWRShaderHandler>(GOW::TypeId::ShaderLibrary));
    return true;
}();

} // namespace GOW