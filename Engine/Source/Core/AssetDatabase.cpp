#include <Onyx/Services/AssetDatabase.h>
#include <Onyx/Vfs/OsFile.h>
#include <Onyx/Vfs/SliceFile.h>
#include <Onyx/Vfs/IsoFileSystem.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Services/TaskManager.h>
#include <Onyx/Services/Events.h>
#include <Onyx/Types/TypeRegistry.h>

namespace Onyx::Services {

// â”€â”€ LoadPakFromIso â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Abre ISO, detecta profile, extrai TOC â†’ paks[]
bool AssetDatabase::LoadPakFromIso(const fs::path& isoPath) {
    if (!fs::exists(isoPath)) return false;

    auto profile = Onyx::Services::ProfileManager::Get().DetectProfileForFile(isoPath);
    if (!profile) return false;

    auto vfs = profile->MountArchive(isoPath);
    if (!vfs) return false;

    AssetContainer pak;
    pak.filename = isoPath.filename().string();
    pak.fullPath = isoPath.string();
    pak.profile  = profile;

    if (profile->LoadFromArchive(vfs, pak)) {
        paks.push_back(std::move(pak));
        Onyx::Services::TaskManager::doLater([this]() {
            if (!paks.empty()) EventPakOpened::post(&paks.back());
        });
        return true;
    }
    return false;
}

void AssetDatabase::ClosePak(size_t idx) {
    if (idx < paks.size())
        paks.erase(paks.begin() + idx);
}

// â”€â”€ LoadWadFromPakEntry â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Abre o PAK dentro da ISO, corta um SliceFile para o entry, ParseContainer â†’ wads[]
bool AssetDatabase::LoadWadFromPakEntry(AssetEntry* e, AssetContainer& parentPak) {
    if (!e || !parentPak.profile) return false;

    auto profile = parentPak.profile;

    // Montar o VFS da ISO de volta
    auto vfs = profile->MountArchive(parentPak.fullPath);
    if (!vfs) {
        LOG_ERR("[AssetDatabase] Could not re-mount ISO: %s", parentPak.fullPath.c_str());
        return false;
    }

    // Testar mÃºltiplos caminhos dentro da ISO para encontrar o PAK
    // e->wadName pode ser "PART1.PAK", "PART2.PAK" etc.
    std::vector<std::string> tryPaths = {
        "/" + e->wadName,
        e->wadName,
    };

    std::unique_ptr<Onyx::Vfs::IFile> partFile;
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
    auto slice = std::make_shared<Onyx::Vfs::SliceFile>(std::move(partFile), e->offset, e->size);

    AssetContainer result;
    result.filename = e->name;
    result.fullPath = parentPak.fullPath;
    result.profile  = profile;
    result.fileSource = slice; // Cache the source stream

    if (profile->ParseContainer(slice, result)) {
        LOG_INFO("[AssetDatabase] Parsed WAD '%s': %zu tags", e->name.c_str(), result.entries.size());
        wads.push_back(std::move(result));
        Onyx::Services::TaskManager::doLater([this]() {
            if (!wads.empty()) EventWadOpened::post(&wads.back());
        });
        return true;
    }

    LOG_ERR("[AssetDatabase] ParseContainer failed for '%s'", e->name.c_str());
    return false;
}

// â”€â”€ OpenPakEntryAsFile â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Returns a SliceFile for a PAK entry without parsing as WAD
std::shared_ptr<Onyx::Vfs::IFile> AssetDatabase::OpenPakEntryAsFile(AssetEntry* e, AssetContainer& parentPak) {
    if (!e || !parentPak.profile) return nullptr;

    auto vfs = parentPak.profile->MountArchive(parentPak.fullPath);
    if (!vfs) {
        LOG_ERR("[AssetDatabase] Could not re-mount ISO: %s", parentPak.fullPath.c_str());
        return nullptr;
    }

    std::vector<std::string> tryPaths = { "/" + e->wadName, e->wadName };
    std::unique_ptr<Onyx::Vfs::IFile> partFile;
    for (auto& p : tryPaths) {
        partFile = vfs->OpenFile(p);
        if (partFile && partFile->IsValid()) break;
        partFile.reset();
    }

    if (!partFile) {
        LOG_ERR("[AssetDatabase] Could not open %s inside ISO.", e->wadName.c_str());
        return nullptr;
    }

    return std::make_shared<Onyx::Vfs::SliceFile>(std::move(partFile), e->offset, e->size);
}

// â”€â”€ LoadWad â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Carrega um .wad solto do disco (nÃ£o da ISO)
bool AssetDatabase::LoadWad(const fs::path& path, const std::string& gameHint) {
    if (!fs::exists(path)) return false;

    // Se for ISO, redirecionar para LoadPakFromIso
    auto ext = path.extension().string();
    if (ext == ".iso" || ext == ".ISO") {
        return LoadPakFromIso(path);
    }

    // Selecionar profile: por hint explÃ­cito ou auto-detect
    std::shared_ptr<Onyx::Domain::IAssetProfile> profile;
    if (!gameHint.empty()) {
        profile = Onyx::Services::ProfileManager::Get().FindProfileByHint(gameHint);
    }
    if (!profile) {
        profile = Onyx::Services::ProfileManager::Get().DetectProfileForFile(path);
    }
    if (!profile) return false;

    // Let the profile do any pre-parse setup before ParseContainer runs.
    profile->PrepareForParse(path);

    AssetContainer wad;
    wad.filename = path.filename().string();
    wad.fullPath = path.string();
    wad.profile  = profile;

    auto file = std::make_shared<Onyx::Vfs::OsFile>(path.string());
    if (!file->IsValid()) return false;

    wad.fileSource = file;

    if (profile->ParseContainer(file, wad)) {
        wads.push_back(std::move(wad));
        Onyx::Services::TaskManager::doLater([this]() {
            if (!wads.empty()) EventWadOpened::post(&wads.back());
        });
        return true;
    }

    return false;
}

void AssetDatabase::CloseWad(size_t idx) {
    if (idx < wads.size()) {
        EventWadClosed::post(idx);
        wads.erase(wads.begin() + idx);
    }
}

void AssetDatabase::CloseAll() {
    paks.clear();
    wads.clear();
    isos.clear();
    EventAllClosed::post();
}

// â”€â”€ ISO â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool AssetDatabase::LoadIso(const fs::path& path) {
    if (!fs::exists(path)) return false;
    auto vfs = std::make_shared<Onyx::Vfs::IsoFileSystem>(path.string());
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

// â”€â”€ EnsureNodeData â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool AssetDatabase::EnsureNodeData(AssetEntry* e, AssetContainer& parentWad) {
    if (!e) return false;
    if (e->assetNode) return true;

    if (!parentWad.profile || !parentWad.fileSource) {
        LOG_ERR("[AssetDatabase] Cannot load data for '%s', parent WAD has no file handle.", e->name.c_str());
        return false;
    }

    if (auto* handler = Onyx::Types::TypeRegistry::Get().Resolve(e->typeId)) {
        auto sliceWindow = std::make_shared<Onyx::Vfs::SliceFile>(parentWad.fileSource, e->offset, e->size);
        e->assetNode = handler->Parse(sliceWindow);
    }

    if (e->assetNode) return true;

    // Most types don't have struct schemas yet. Only log at debug level to avoid spam.
    LOG_DEBUG("[AssetDatabase] No schema/handler for '%s' (TypeId: %d)", e->name.c_str(), (int)e->typeId.value);
    return false;
}

// â”€â”€ Asynchronous Loading (via TaskManager) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void AssetDatabase::LoadWadAsync(const fs::path& path, const std::string& gameHint) {
    if (IsLoading()) {
        LOG_WARN("[AssetDatabase] A load operation is already in progress.");
        return;
    }

    Onyx::Services::TaskManager::createTask("Loading " + path.filename().string(), 100, [this, path, gameHint](Onyx::Services::Task& task) {
        task.update(5);

        bool success = LoadWad(path, gameHint);

        task.update(90);

        // Event emission is now handled internally by LoadWad via doLater

        task.update(100);
    });
}

void AssetDatabase::LoadIsoPakAsync(const fs::path& path) {
    if (IsLoading()) {
        LOG_WARN("[AssetDatabase] A load operation is already in progress.");
        return;
    }

    Onyx::Services::TaskManager::createTask("Loading " + path.filename().string(), 100, [this, path](Onyx::Services::Task& task) {
        task.update(10);

        bool isoSuccess = LoadIso(path);
        task.update(40);

        if (isoSuccess) {
            bool pakSuccess = LoadPakFromIso(path);
            task.update(90);

            // Event emission is now handled internally by LoadPakFromIso via doLater
        }

        task.update(100);
    });
}

bool AssetDatabase::IsLoading() const {
    return Onyx::Services::TaskManager::getRunningTaskCount() > 0;
}

} // namespace Onyx::Services

