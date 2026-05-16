#include "AssetDatabase.h"
#include "vfs/OsFile.h"
#include "vfs/SliceFile.h"
#include "vfs/IsoFileSystem.h"
#include "core/Logger.h"
#include "core/TaskManager.h"
#include "core/Events.h"
#include "types/TypeRegistry.h"
#include <GLFW/glfw3.h> // For glfwPostEmptyEvent
#include <thread>
#include <chrono>

// ── LoadPakFromIso ─────────────────────────────────────────────────────────
// Abre ISO, detecta profile, extrai TOC → paks[]
bool AssetDatabase::LoadPakFromIso(const fs::path& isoPath) {
    if (!fs::exists(isoPath)) return false;

    auto profile = GOW::ProfileManager::Get().DetectProfileForFile(isoPath);
    if (!profile) return false;

    auto vfs = profile->MountArchive(isoPath);
    if (!vfs) return false;

    OpenWad pak;
    pak.filename = isoPath.filename().string();
    pak.fullPath = isoPath.string();
    pak.profile  = profile;

    if (profile->LoadFromArchive(vfs, pak)) {
        paks.push_back(std::move(pak));
        return true;
    }
    return false;
}

void AssetDatabase::ClosePak(size_t idx) {
    if (idx < paks.size())
        paks.erase(paks.begin() + idx);
}

// ── LoadWadFromPakEntry ────────────────────────────────────────────────────
// Abre o PAK dentro da ISO, corta um SliceFile para o entry, ParseWad → wads[]
bool AssetDatabase::LoadWadFromPakEntry(ParsedEntry* e, OpenWad& parentPak) {
    if (!e || !parentPak.profile) return false;

    auto profile = parentPak.profile;

    // Montar o VFS da ISO de volta
    auto vfs = profile->MountArchive(parentPak.fullPath);
    if (!vfs) {
        LOG_ERR("[AssetDatabase] Could not re-mount ISO: %s", parentPak.fullPath.c_str());
        return false;
    }

    // Testar múltiplos caminhos dentro da ISO para encontrar o PAK
    // e->wadName pode ser "PART1.PAK", "PART2.PAK" etc.
    std::vector<std::string> tryPaths = {
        "/" + e->wadName,
        e->wadName,
    };

    std::unique_ptr<GOW::IFile> partFile;
    for (auto& p : tryPaths) {
        partFile = vfs->OpenFile(p);
        if (partFile && partFile->IsValid()) {
            LOG_INFO("[AssetDatabase] Found %s in ISO", p.c_str());
            break;
        }
        partFile.reset();
    }

    if (!partFile) {
        LOG_ERR("[AssetDatabase] Could not open %s inside ISO.", e->wadName.c_str());
        return false;
    }

    // Criar um slice para o offset/size deste entry dentro do PAK
    auto slice = std::make_shared<GOW::SliceFile>(std::move(partFile), e->offset, e->size);

    OpenWad result;
    result.filename = e->name;
    result.fullPath = parentPak.fullPath;
    result.profile  = profile;
    result.fileSource = slice; // Cache the source stream

    if (profile->ParseWad(slice, result)) {
        LOG_INFO("[AssetDatabase] Parsed WAD '%s': %zu tags", e->name.c_str(), result.entries.size());
        wads.push_back(std::move(result));
        return true;
    }

    LOG_ERR("[AssetDatabase] ParseWad failed for '%s'", e->name.c_str());
    return false;
}

// ── OpenPakEntryAsFile ────────────────────────────────────────────────────
// Returns a SliceFile for a PAK entry without parsing as WAD
std::shared_ptr<GOW::IFile> AssetDatabase::OpenPakEntryAsFile(ParsedEntry* e, OpenWad& parentPak) {
    if (!e || !parentPak.profile) return nullptr;

    auto vfs = parentPak.profile->MountArchive(parentPak.fullPath);
    if (!vfs) {
        LOG_ERR("[AssetDatabase] Could not re-mount ISO: %s", parentPak.fullPath.c_str());
        return nullptr;
    }

    std::vector<std::string> tryPaths = { "/" + e->wadName, e->wadName };
    std::unique_ptr<GOW::IFile> partFile;
    for (auto& p : tryPaths) {
        partFile = vfs->OpenFile(p);
        if (partFile && partFile->IsValid()) break;
        partFile.reset();
    }

    if (!partFile) {
        LOG_ERR("[AssetDatabase] Could not open %s inside ISO.", e->wadName.c_str());
        return nullptr;
    }

    return std::make_shared<GOW::SliceFile>(std::move(partFile), e->offset, e->size);
}

// ── LoadWad ────────────────────────────────────────────────────────────────
// Carrega um .wad solto do disco (não da ISO)
bool AssetDatabase::LoadWad(const fs::path& path, const std::string& gameHint) {
    if (!fs::exists(path)) return false;

    // Se for ISO, redirecionar para LoadPakFromIso
    auto ext = path.extension().string();
    if (ext == ".iso" || ext == ".ISO") {
        return LoadPakFromIso(path);
    }

    // Selecionar profile: por hint explícito ou auto-detect
    std::shared_ptr<GOW::IGameProfile> profile;
    if (!gameHint.empty()) {
        profile = GOW::ProfileManager::Get().FindProfileByHint(gameHint);
    }
    if (!profile) {
        profile = GOW::ProfileManager::Get().DetectProfileForFile(path);
    }
    if (!profile) return false;

    OpenWad wad;
    wad.filename = path.filename().string();
    wad.fullPath = path.string();
    wad.profile  = profile;

    auto file = std::make_shared<GOW::OsFile>(path.string());
    if (!file->IsValid()) return false;
    
    wad.fileSource = file;

    if (profile->ParseWad(file, wad)) {
        wads.push_back(std::move(wad));
        return true;
    }

    return false;
}

void AssetDatabase::CloseWad(size_t idx) {
    if (idx < wads.size())
        wads.erase(wads.begin() + idx);
}

void AssetDatabase::CloseAll() {
    paks.clear();
    wads.clear();
    isos.clear();
    EventAllClosed::post();
}

// ── ISO ────────────────────────────────────────────────────────────────────
bool AssetDatabase::LoadIso(const fs::path& path) {
    if (!fs::exists(path)) return false;
    auto vfs = std::make_shared<GOW::IsoFileSystem>(path.string());
    if (vfs->Initialize()) {
        isos.push_back(vfs);
        return true;
    }
    return false;
}

void AssetDatabase::CloseIso(size_t idx) {
    if (idx < isos.size())
        isos.erase(isos.begin() + idx);
}

// ── EnsureNodeData ─────────────────────────────────────────────────────────
bool AssetDatabase::EnsureNodeData(ParsedEntry* e, OpenWad& parentWad) {
    if (!e) return false;
    if (e->assetNode) return true;

    if (!parentWad.profile || !parentWad.fileSource) {
        LOG_ERR("[AssetDatabase] Cannot load data for '%s', parent WAD has no file handle.", e->name.c_str());
        return false;
    }

    if (auto* handler = GOW::TypeRegistry::Get().Resolve(e->typeId)) {
        auto sliceWindow = std::make_shared<GOW::SliceFile>(parentWad.fileSource, e->offset, e->size);
        e->assetNode = handler->Parse(sliceWindow);
    }

    if (e->assetNode) return true;

    // Most types don't have struct schemas yet. Only log at debug level to avoid spam.
    LOG_DEBUG("[AssetDatabase] No schema/handler for '%s' (TypeId: %d)", e->name.c_str(), (int)e->typeId);
    return false;
}

// ── Asynchronous Loading ───────────────────────────────────────────────────

void AssetDatabase::LoadWadAsync(const fs::path& path, const std::string& gameHint) {
    if (m_loadState.load() == LoadState::LoadingWad || m_loadState.load() == LoadState::LoadingIsoPak) {
        LOG_WARN("[AssetDatabase] A load operation is already in progress.");
        return;
    }
    
    m_loadState.store(LoadState::LoadingWad);
    m_loadMessage = "Loading WAD: " + path.filename().string();
    m_loadProgress.store(0.0f);

    GOW::TaskManager::createTask("Loading " + path.filename().string(), 100, [this, path, gameHint](GOW::Task& task) {
        // Initial UI tick
        task.update(5);
        glfwPostEmptyEvent();

        bool success = LoadWad(path, gameHint);

        task.update(90);
        glfwPostEmptyEvent();

        // Switch to main thread for state update and event emission
        GOW::TaskManager::doLater([this, success]() {
            if (success) {
                m_loadState.store(LoadState::Ready);
                if (!wads.empty())
                    EventWadOpened::post(&wads.back());
            } else {
                m_loadState.store(LoadState::Error);
            }
        });

        task.update(100);
        glfwPostEmptyEvent();
    });
}

void AssetDatabase::LoadIsoPakAsync(const fs::path& path) {
     if (m_loadState.load() == LoadState::LoadingWad || m_loadState.load() == LoadState::LoadingIsoPak) {
        LOG_WARN("[AssetDatabase] A load operation is already in progress.");
        return;
    }

    m_loadState.store(LoadState::LoadingIsoPak);
    m_loadMessage = "Loading ISO & PAK: " + path.filename().string();
    m_loadProgress.store(0.0f);

    GOW::TaskManager::createTask("Loading " + path.filename().string(), 100, [this, path](GOW::Task& task) {
        task.update(10);
        glfwPostEmptyEvent();

        bool isoSuccess = LoadIso(path);
        task.update(40);
        glfwPostEmptyEvent();

        if (isoSuccess) {
            bool pakSuccess = LoadPakFromIso(path);
            task.update(90);
            glfwPostEmptyEvent();

            GOW::TaskManager::doLater([this, pakSuccess]() {
                if (pakSuccess) {
                    m_loadState.store(LoadState::Ready);
                    if (!paks.empty())
                        EventPakOpened::post(&paks.back());
                } else {
                    m_loadState.store(LoadState::Error);
                }
            });
        } else {
            GOW::TaskManager::doLater([this]() {
                m_loadState.store(LoadState::Error);
            });
        }

        task.update(100);
        glfwPostEmptyEvent();
    });
}

void AssetDatabase::UpdateAsyncLoadStatus() {
    // Legacy: check future state (for any remaining std::async callers)
    auto state = m_loadState.load();
    if (state == LoadState::None) return;

    if (m_pendingLoad.valid()) {
        auto status = m_pendingLoad.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            m_pendingLoad.get();
        }
    }
}

