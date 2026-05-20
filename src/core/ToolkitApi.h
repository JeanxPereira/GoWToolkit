#pragma once

// ToolkitApi — Global singleton facade for accessing core subsystems.
// Inspired by ImHexApi::System — provides global access without AppContext& pass-through.

class AssetDatabase;
class AppConfig;

namespace GOW {
    class ProfileManager;
    class TypeRegistry;
    class ViewerRegistry;
    class DocumentWindow;
}

struct ParsedEntry;
struct OpenWad;

namespace GOW::Api {

    struct InitParams {
        AssetDatabase*       db = nullptr;
        AppConfig*           config = nullptr;
        GOW::ViewerRegistry* viewers = nullptr;
        GOW::DocumentWindow* documents = nullptr;
    };

    /// Initialize the facade pointers. Call once in App::init().
    void Init(const InitParams& params);

    /// Access the global AssetDatabase.
    AssetDatabase& Database();

    /// Access the global AppConfig.
    AppConfig& Config();

    /// Access the global ProfileManager.
    ProfileManager& Profiles();

    /// Access the global TypeRegistry.
    TypeRegistry& Types();

    /// Access the global ViewerRegistry.
    ViewerRegistry& Viewers();

    /// Access the global DocumentWindow.
    DocumentWindow& Documents();

    /// Access the currently selected entry in the active WAD/PAK.
    ParsedEntry* GetSelected();

    /// Access the WAD containing the currently selected entry.
    OpenWad* GetSelectedWad();

    /// Set the globally selected entry and post EventAssetSelected.
    void SetSelected(ParsedEntry* entry, OpenWad* wad);

} // namespace GOW::Api
