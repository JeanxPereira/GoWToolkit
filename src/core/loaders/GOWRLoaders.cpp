#include "core/loaders/GOWRLoaders.h"
#include "core/types/TypeRegistry.h"
#include "core/WadTypes.h"
#include "core/schema/AssetReader.h"
#include "core/formats/GOWRMeshDefnFormat.h"
#include "core/parsers/gowr/MeshParser.h"
#include "core/parsers/gowr/LodPackIndex.h"
#include "core/Logger.h"
#include "core/vfs/SliceFile.h"
#include "core/vfs/MemoryFile.h"
#include "ui/viewers/Viewport3D.h"
#include <filesystem>
#include <functional>
#include <fstream>

// ── GOWRLoaders.cpp ────────────────────────────────────────────────────────

namespace GOW {

// ── Resolve game root from config.ini ─────────────────────────────────────
// Mirrors the C# extractor: reads config.ini next to the exe.
// Line 0: comment/header, Line 1: "gameroot=<path>"
static std::filesystem::path ReadGameRootFromConfig() {
    // Resolve exe directory via argv[0] equivalent: use current path as fallback
    auto exeDir = std::filesystem::current_path();
    auto configPath = exeDir / "config.ini";

    std::ifstream cfg(configPath);
    if (!cfg.is_open()) return {};

    std::string line;
    std::getline(cfg, line); // skip first line (comment)
    if (!std::getline(cfg, line)) return {};

    // Expect "gameroot=D:\..." or just the path on line 1
    auto eq = line.find('=');
    if (eq != std::string::npos)
        line = line.substr(eq + 1);

    // Trim whitespace
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
        line.pop_back();

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

void InvalidateLodIndex() {
    delete s_lodIndex;
    s_lodIndex = nullptr;
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

REGISTER_FILE_TYPE(GOWRMeshDefnHandler);
REGISTER_FILE_TYPE(GOWRSkinnedMeshHandler);
REGISTER_FILE_TYPE(GOWRModelInstanceHandler);

} // namespace GOW