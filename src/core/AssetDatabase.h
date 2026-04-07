#pragma once
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <future>
#include <atomic>
#include "interfaces/IGameProfile.h"
#include "ProfileManager.h"
#include "schema/NodeInstance.h"

namespace fs = std::filesystem;

#include "WadTypes.h"

namespace GOW {
    class IsoFileSystem;
}

class AssetDatabase {
public:
    // Carrega ISO e extrai TOC -> paks
    bool LoadPakFromIso(const fs::path& isoPath);
    void ClosePak(size_t idx);

    // Carrega WAD interno a partir de um entry do PAK
    bool LoadWadFromPakEntry(ParsedEntry* e, OpenWad& parentPak);

    // Get a file handle for a PAK entry without parsing as WAD
    std::shared_ptr<GOW::IFile> OpenPakEntryAsFile(ParsedEntry* e, OpenWad& parentPak);

    // Carrega WAD solto de disco (gameHint: "ragnarok", "gow2", etc.)
    bool LoadWad(const fs::path& path, const std::string& gameHint = "");
    void CloseWad(size_t idx);

    void CloseAll();

    // Carregamento lazy de dados de uma entry
    bool EnsureNodeData(ParsedEntry* e, OpenWad& parentWad);

    // ISOs carregados (para IsoBrowser)
    bool LoadIso(const fs::path& path);
    void CloseIso(size_t idx);

    // ── Asynchronous Loading ──
    enum class LoadState { None, LoadingWad, LoadingIsoPak, Ready, Error };
    
    // Non-blocking wrapper for LoadWad (updates state and invokes std::async)
    void LoadWadAsync(const fs::path& path, const std::string& gameHint = "");
    // Non-blocking wrapper for LoadIso + LoadPakFromIso
    void LoadIsoPakAsync(const fs::path& path);

    // Check loop to be called in `App::frame`
    void UpdateAsyncLoadStatus();

    std::atomic<float> m_loadProgress{0.0f};
    std::atomic<LoadState> m_loadState{LoadState::None};
    std::string m_loadMessage = "";

    // ── Data ──
    std::vector<OpenWad> paks;  // TOC entries (arquivos dentro de ISOs/PAKs)
    std::vector<OpenWad> wads;  // WAD internals (tags parseadas de um .WAD)
    std::vector<std::shared_ptr<GOW::IsoFileSystem>> isos;

private:
    std::future<void> m_pendingLoad;
};

