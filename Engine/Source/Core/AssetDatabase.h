#pragma once
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include "interfaces/IGameProfile.h"
#include "ProfileManager.h"
#include "schema/NodeInstance.h"

namespace fs = std::filesystem;

#include "domain/Wad.h"
#include "types/GameVersion.h"
#include "types/TypeId.h"

namespace Onyx {
    class IsoFileSystem;
}

class AssetDatabase {
public:
    // Carrega ISO e extrai TOC -> paks
    bool LoadPakFromIso(const fs::path& isoPath);
    void ClosePak(size_t idx);

    // Carrega WAD interno a partir de um entry do PAK
    bool LoadWadFromPakEntry(AssetEntry* e, AssetContainer& parentPak);

    // Get a file handle for a PAK entry without parsing as WAD
    std::shared_ptr<Onyx::IFile> OpenPakEntryAsFile(AssetEntry* e, AssetContainer& parentPak);

    // Carrega WAD solto de disco (gameHint: "ragnarok", "gow2", etc.)
    bool LoadWad(const fs::path& path, const std::string& gameHint = "");
    void CloseWad(size_t idx);

    void CloseAll();

    // Carregamento lazy de dados de uma entry
    bool EnsureNodeData(AssetEntry* e, AssetContainer& parentWad);

    // ISOs carregados (para IsoBrowser)
    bool LoadIso(const fs::path& path);
    void CloseIso(size_t idx);

    // ── Asynchronous Loading (via TaskManager) ──

    // Non-blocking wrapper for LoadWad (creates a TaskManager task)
    void LoadWadAsync(const fs::path& path, const std::string& gameHint = "");
    // Non-blocking wrapper for LoadIso + LoadPakFromIso
    void LoadIsoPakAsync(const fs::path& path);

    // Returns true if any foreground load task is currently running.
    bool IsLoading() const;

    // ── Data ──
    std::vector<AssetContainer> paks;  // TOC entries (arquivos dentro de ISOs/PAKs)
    std::vector<AssetContainer> wads;  // WAD internals (tags parseadas de um .WAD)
    std::vector<std::shared_ptr<Onyx::IsoFileSystem>> isos;
};

