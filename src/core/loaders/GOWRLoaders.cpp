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

static std::shared_ptr<IDocumentContent> SharedGowrMeshLoad(const ParsedEntry& entry, OpenWad& wad) {
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

    // ── Find paired goProto* file (skeleton) ───────────────────────────
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

    std::shared_ptr<ObjectData> skeleton;
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

    // ── Build SceneData and load into viewport ─────────────────────────
    auto vp = std::make_shared<Viewport3D>(entry.name);

    if (skeleton) {
        // ── Rigid skinning per submesh: read MG, assign parentBone, override
        //    per-vertex bone indices/weights with (1.0, bone0).
        if (mgFile) {
            // We need the MESH-file submesh count to size the parentBone table.
            // The mesh parser stores parts in the same order as MESH submeshes
            // it produced; their materialId encodes the original submesh index.
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
    return SharedGowrMeshLoad(entry, wad);
}
std::shared_ptr<IDocumentContent> GOWRSkinnedMeshHandler::CreateViewer(const ParsedEntry& entry, OpenWad& wad) {
    return SharedGowrMeshLoad(entry, wad);
}
std::shared_ptr<IDocumentContent> GOWRModelInstanceHandler::CreateViewer(const ParsedEntry& entry, OpenWad& wad) {
    return SharedGowrMeshLoad(entry, wad);
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
    // Rigs don't have a standalone viewer yet.
    return nullptr;
}

REGISTER_FILE_TYPE(GOWRMeshDefnHandler);
REGISTER_FILE_TYPE(GOWRSkinnedMeshHandler);
REGISTER_FILE_TYPE(GOWRModelInstanceHandler);
REGISTER_FILE_TYPE(GOWRTextureHandler);
REGISTER_FILE_TYPE(GOWRRigHandler);

} // namespace GOW