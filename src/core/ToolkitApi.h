#pragma once

// ToolkitApi — Global singleton facade for accessing core subsystems.
// Inspired by ImHexApi::System — provides global access without AppContext& pass-through.

class AssetDatabase;
class AppConfig;

namespace GOW {
    class ProfileManager;
    class TypeRegistry;
}

namespace GOW::Api {

    /// Initialize the facade pointers. Call once in App::init().
    void Init(AssetDatabase* db, AppConfig* config);

    /// Access the global AssetDatabase.
    AssetDatabase& Database();

    /// Access the global AppConfig.
    AppConfig& Config();

} // namespace GOW::Api
