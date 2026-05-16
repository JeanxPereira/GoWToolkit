#include "core/loaders/GOWRLoaders.h"
#include "core/types/TypeRegistry.h"
#include "core/WadTypes.h"
#include "core/schema/AssetReader.h"
#include "core/formats/GOWRMeshDefnFormat.h"
#include "core/parsers/gowr/MeshParser.h"
#include "core/parsers/gowr/LodPackIndex.h"
#include "core/parsers/gowr/TexPackIndex.h"
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

// ── Resolve game root from config.ini ─────────────────────────────────────
static std::filesystem::path ReadGameRootFromConfig() {
    std::vector<std::filesystem::path> candidates;
    
    // 1. Current working directory
    auto cwd = std::filesystem::current_path();
    candidates.push_back(cwd);
    LOG_INFO("[GOWRLoaders] CWD: %s", cwd.string().c_str());
    
#ifdef __APPLE__
    // 2. macOS: resolve exe path from _NSGetExecutablePath
    {
        char exeBuf[4096] = {};
        uint32_t bufSize = sizeof(exeBuf);
        if (_NSGetExecutablePath(exeBuf, &bufSize) == 0) {
            try {
                auto exePath = std::filesystem::path(exeBuf);
                auto exeDir = exePath.parent_path();
                candidates.push_back(exeDir);
                LOG_INFO("[GOWRLoaders] Exe dir: %s", exeDir.string().c_str());
                
                // Walk up: MacOS -> Contents -> .app -> build/
                auto buildDir = exeDir.parent_path().parent_path().parent_path();
                candidates.push_back(buildDir);
                LOG_INFO("[GOWRLoaders] Build dir: %s", buildDir.string().c_str());
            } catch (const std::exception& e) {
                LOG_WARN("[GOWRLoaders] Failed to resolve exe path: %s", e.what());
            }
        } else {
            LOG_WARN("[GOWRLoaders] _NSGetExecutablePath failed");
        }
    }
#endif
    
    // 3. Log all candidates and try each
    std::ifstream cfg;
    std::filesystem::path configPath;
    for (const auto& dir : candidates) {
        configPath = dir / "config.ini";
        LOG_INFO("[GOWRLoaders] Trying: %s", configPath.string().c_str());
        cfg.open(configPath);
        if (cfg.is_open()) {
            LOG_INFO("[GOWRLoaders] Found config.ini at: %s", configPath.string().c_str());
            break;
        }
    }
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
        auto exeDir      = std::filesystem::current_path();
        auto lodpacksTxt = exeDir / "lodpacks.txt";
        auto gameRoot    = ReadGameRootFromConfig();

        if (!gameRoot.empty() && std::filesystem::exists(lodpacksTxt)) {
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

    // ── Upload to viewport ─────────────────────────────────────────────
    auto vp = std::make_shared<Viewport3D>(entry.name);
    std::vector<std::unique_ptr<TextureData>> noTextures; // textures wired later
    vp->LoadFromMeshData(data, noTextures);
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