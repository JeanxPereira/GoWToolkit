#include "core/ToolkitApi.h"
#include <cstdio>
#include <cstdlib>

namespace GOW::Api {

    static AssetDatabase* s_database = nullptr;
    static AppConfig*     s_config   = nullptr;

    void Init(AssetDatabase* db, AppConfig* config) {
        s_database = db;
        s_config   = config;
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

} // namespace GOW::Api
