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
        
        // 1. Handle TOC Loading State
        if (!texIdx.IsLoaded()) {
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x * 0.5f - 150, ImGui::GetWindowSize().y * 0.5f - 20));
            ImGui::Text("Loading Texture Index (TOCs) in background...");
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.5f - 150);
            ImGui::ProgressBar(texIdx.GetLoadProgress(), ImVec2(300, 0));
            return;
        }

        // 2. Initial Load
        if (!m_initialized) {
            m_initialized = true;
            FirstLoad(texIdx);
        }

        // 3. Main Splitter
        if (m_rawTiledData.empty()) {
            ImGui::TextColored(ImVec4(1,0,0,1), "Texture load failed or missing raw data.");
            return;
        }

        // Draw Live Debugger on the left, Viewer on the right
        ImGui::BeginChild("TexDebugger", ImVec2(280, 0), true);
        ImGui::Text("RDNA2 Detile Debugger");
        ImGui::Separator();
        
        bool changed = false;
        ImGui::Text("Size: %ux%u", m_texW, m_texH);
        ImGui::Text("Format: %u %s", m_fmt, m_isBc ? "(BC)" : "(Raw)");
        ImGui::Text("BlockBytes: %u", m_blockBytes);
        
        ImGui::Separator();
        ImGui::Text("Descriptor Info:");
        ImGui::Text("Orig SwMode: %u", m_origSwMode);
        ImGui::Text("Orig PipeXor: %u", m_origPipeBankXor);
        
        ImGui::Separator();
        ImGui::Text("Tweak Parameters:");
        
        changed |= ImGui::InputInt("SW_MODE", &m_debugSwMode);
        changed |= ImGui::InputInt("PipeBankXor", &m_debugPipeBankXor);
        
        int blockSizes[] = {256, 4096, 65536};
        const char* blockNames[] = {"256B", "4KB", "64KB"};
        if (ImGui::Combo("Macro Block", &m_debugMacroIdx, blockNames, 3)) changed = true;
        
        if (ImGui::Button("Apply Detile", ImVec2(-1, 30)) || changed) {
            RefreshTexture();
        }
        
        if (ImGui::Button("Reset to Original", ImVec2(-1, 30))) {
            m_debugSwMode = m_origSwMode;
            m_debugPipeBankXor = m_origPipeBankXor;
            m_debugMacroIdx = 1; // 4KB default
            RefreshTexture();
        }
        
        ImGui::EndChild();
        
        ImGui::SameLine();
        ImGui::BeginChild("TexView");
        if (m_realViewer) {
            m_realViewer->Draw();
        } else {
            ImGui::Text("Failed to detile/decompress with these parameters.");
        }
        ImGui::EndChild();
    }

private:
    ParsedEntry m_entry;
    std::string m_name;
    uint64_t m_hash = 0;
    bool m_initialized = false;
    std::shared_ptr<ImageViewer> m_realViewer;
    
    std::vector<uint8_t> m_rawTiledData;
    uint32_t m_texW = 0, m_texH = 0;
    uint32_t m_fmt = 0;
    BcFormat m_bcFmt = BcFormat::BC1;
    bool m_isBc = false;
    uint32_t m_blockBytes = 8;
    
    uint32_t m_origSwMode = 0;
    uint32_t m_origPipeBankXor = 0;
    
    int m_debugSwMode = 0;
    int m_debugPipeBankXor = 0;
    int m_debugMacroIdx = 1; // 0=256B, 1=4KB, 2=64KB

    void FirstLoad(TexPackIndex& texIdx) {
        if (!m_hash) return;
        TexpackEntry texEntry;
        if (!texIdx.FindTexture(m_hash, texEntry)) return;
        
        auto file = texIdx.GetFile(texEntry.packIdx);
        if (!file || !file->IsValid()) return;
        
        file->Seek(static_cast<int64_t>(texEntry.blockDataOffset), 0);
        uint32_t bMagic = 0, off = 0, bLen = 0, bUnk = 0;
        file->Read(&bMagic, 4); file->Read(&off, 4); file->Read(&bLen, 4); file->Read(&bUnk, 4);
        
        if (off == 0x20) return;
        
        uint8_t gnfBuf[0x100] = {};
        file->Read(gnfBuf, 0x100);
        
        m_texW = *(uint16_t*)(&gnfBuf[0x06]);
        m_texH = *(uint16_t*)(&gnfBuf[0x08]);
        
        uint32_t dw2C_raw = *reinterpret_cast<uint32_t*>(gnfBuf + 0x2C);
        
        file->Seek(static_cast<int64_t>(texEntry.blockDataOffset) + 0x100 + 0x14 + 0x08, 0);
        uint32_t decSize = 0;
        file->Read(&decSize, 4);
        file->Seek(4, 1);
        
        if (decSize == 0 || decSize > 1024 * 1024 * 64) {
            decSize = dw2C_raw;
            file->Seek(static_cast<int64_t>(texEntry.blockDataOffset) + 0x124, 0);
        }
        
        m_rawTiledData.resize(decSize);
        file->Read(m_rawTiledData.data(), decSize);
        
        uint32_t dw1A = *(uint32_t*)(&gnfBuf[0x40 + 4]);
        uint32_t dw1C = *(uint32_t*)(&gnfBuf[0x40 + 12]);
        
        m_fmt = dw1A & 0x3F;
        m_isBc = GnfFmtToBc(m_fmt, m_bcFmt);
        m_blockBytes = m_isBc ? BcBlockSize(m_bcFmt) : 8;
        
        m_origSwMode = (dw1C >> 20) & 0x1F;
        m_origPipeBankXor = (dw1C >> 8) & 0x3FFF;
        
        m_debugSwMode = m_origSwMode;
        m_debugPipeBankXor = m_origPipeBankXor;
        if (m_origSwMode >= 8 && m_origSwMode <= 11) m_debugMacroIdx = 2;
        else if (m_origSwMode >= 4 && m_origSwMode <= 7) m_debugMacroIdx = 1;
        else if (m_origSwMode >= 1 && m_origSwMode <= 3) m_debugMacroIdx = 0;
        
        RefreshTexture();
    }
    
    void RefreshTexture() {
        if (m_rawTiledData.empty()) return;
        m_realViewer = nullptr; // release old
        
        auto texData = std::make_unique<TextureData>();
        texData->name = m_name;
        texData->width = m_texW;
        texData->height = m_texH;
        
        uint32_t swMode = m_debugSwMode;
        uint32_t appliedXor = m_debugPipeBankXor;
        if (swMode < 16) appliedXor = 0; // standard modes
        
        if (m_isBc) {
            uint32_t blocksX = (m_texW + 3) / 4;
            uint32_t blocksY = (m_texH + 3) / 4;
            size_t linearBcSz = (size_t)blocksX * blocksY * m_blockBytes;
            std::vector<uint8_t> linearBc(linearBcSz, 0);
            
            if (swMode == 0) {
                if (m_rawTiledData.size() >= linearBcSz)
                    std::memcpy(linearBc.data(), m_rawTiledData.data(), linearBcSz);
            } else {
                uint32_t macros[] = {256, 4096, 65536};
                uint32_t macro = macros[m_debugMacroIdx];
                DetileRdna2(m_rawTiledData.data(), m_rawTiledData.size(), linearBc.data(), blocksX, blocksY, m_blockBytes, appliedXor, macro);
            }
            
            std::vector<uint8_t> rgba;
            if (!DecompressBc(linearBc.data(), linearBcSz, m_texW, m_texH, m_bcFmt, rgba)) {
                return;
            }
            texData->isCompressed = false;
            texData->glInternalFormat = 0x1908;
            texData->dataSize = rgba.size();
            texData->pixels = std::move(rgba);
        } else {
            texData->isCompressed = false;
            texData->glInternalFormat = 0x1908;
            texData->dataSize = m_rawTiledData.size();
            texData->pixels = m_rawTiledData; // copy
        }
        
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