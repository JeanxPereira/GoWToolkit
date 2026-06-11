#pragma once

// ToolkitApi — Global singleton facade for accessing core subsystems.
// Inspired by ImHexApi::System — provides global access without AppContext& pass-through.

class AssetDatabase;
class AppConfig;

namespace Onyx {
    class ProfileManager;
    class ViewerRegistry;
}
namespace Onyx::Viewers {
    class DocumentWindow;
}
namespace Onyx::Types { class TypeRegistry; }

namespace Onyx::Domain { struct AssetEntry; struct AssetContainer; }
using AssetEntry    = Onyx::Domain::AssetEntry;
using AssetContainer = Onyx::Domain::AssetContainer;

namespace Onyx::Api {

    struct InitParams {
        AssetDatabase*             db = nullptr;
        AppConfig*                 config = nullptr;
        Onyx::ViewerRegistry*      viewers = nullptr;
        Onyx::Viewers::DocumentWindow* documents = nullptr;
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
    Onyx::Types::TypeRegistry& Types();

    /// Access the global ViewerRegistry.
    ViewerRegistry& Viewers();

    /// Access the global DocumentWindow.
    Viewers::DocumentWindow& Documents();

    /// Access the currently selected entry in the active WAD/PAK.
    AssetEntry* GetSelected();

    /// Access the WAD containing the currently selected entry.
    AssetContainer* GetSelectedWad();

    /// Set the globally selected entry and post EventAssetSelected.
    void SetSelected(AssetEntry* entry, AssetContainer* wad);

} // namespace Onyx::Api
