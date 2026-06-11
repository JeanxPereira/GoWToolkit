#include "Core/ToolkitApi.h"
#include "Core/ProfileManager.h"
#include "Core/Types/TypeRegistry.h"
#include "Ui/ViewerRegistry.h"
#include "Ui/Viewers/DocumentWindow.h"
#include "Core/Events.h"
#include <cstdio>
#include <cstdlib>

namespace Onyx::Api {

    static AssetDatabase*       s_database  = nullptr;
    static AppConfig*           s_config    = nullptr;
    static Onyx::ViewerRegistry* s_viewers   = nullptr;
    static Onyx::DocumentWindow* s_documents = nullptr;

    static AssetEntry*         s_selectedEntry = nullptr;
    static AssetContainer*             s_selectedWad   = nullptr;

    void Init(const InitParams& params) {
        s_database  = params.db;
        s_config    = params.config;
        s_viewers   = params.viewers;
        s_documents = params.documents;
    }

    AssetDatabase& Database() {
        if (!s_database) {
            fprintf(stderr, "[ToolkitApi] FATAL: Database() called before Init()\n");
            std::abort();
        }
        return *s_database;
    }

    AppConfig& Config() {
        if (!s_config) {
            fprintf(stderr, "[ToolkitApi] FATAL: Config() called before Init()\n");
            std::abort();
        }
        return *s_config;
    }

    ProfileManager& Profiles() {
        return ProfileManager::Get();
    }

    Onyx::Types::TypeRegistry& Types() {
        return Onyx::Types::TypeRegistry::Get();
    }

    ViewerRegistry& Viewers() {
        if (!s_viewers) {
            fprintf(stderr, "[ToolkitApi] FATAL: Viewers() called before Init() or missing param\n");
            std::abort();
        }
        return *s_viewers;
    }

    DocumentWindow& Documents() {
        if (!s_documents) {
            fprintf(stderr, "[ToolkitApi] FATAL: Documents() called before Init() or missing param\n");
            std::abort();
        }
        return *s_documents;
    }

    AssetEntry* GetSelected() {
        return s_selectedEntry;
    }

    AssetContainer* GetSelectedWad() {
        return s_selectedWad;
    }

    void SetSelected(AssetEntry* entry, AssetContainer* wad) {
        s_selectedEntry = entry;
        s_selectedWad   = wad;
        EventAssetSelected::post(entry, wad);
    }

} // namespace Onyx::Api
