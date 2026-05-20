#include "core/ToolkitApi.h"
#include "core/ProfileManager.h"
#include "core/types/TypeRegistry.h"
#include "ui/ViewerRegistry.h"
#include "ui/viewers/DocumentWindow.h"
#include "core/Events.h"
#include <cstdio>
#include <cstdlib>

namespace GOW::Api {

    static AssetDatabase*       s_database  = nullptr;
    static AppConfig*           s_config    = nullptr;
    static GOW::ViewerRegistry* s_viewers   = nullptr;
    static GOW::DocumentWindow* s_documents = nullptr;

    static ParsedEntry*         s_selectedEntry = nullptr;
    static OpenWad*             s_selectedWad   = nullptr;

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

    TypeRegistry& Types() {
        return TypeRegistry::Get();
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

    ParsedEntry* GetSelected() {
        return s_selectedEntry;
    }

    OpenWad* GetSelectedWad() {
        return s_selectedWad;
    }

    void SetSelected(ParsedEntry* entry, OpenWad* wad) {
        s_selectedEntry = entry;
        s_selectedWad   = wad;
        EventAssetSelected::post(entry, wad);
    }

} // namespace GOW::Api
