#pragma once

// GOWToolkit Event Catalog
// All events used across the application.
// Convention: Event names match the subsystem they belong to.

#include "core/EventManager.h"

// Forward declarations
#include <memory>
struct OpenWad;
struct ParsedEntry;
namespace GOW { class IDocumentContent; }
namespace GOW { struct AnimationData; }
class AppConfig;

// ── Lifecycle Events ───────────────────────────────────────────────────────

/// Fired after App::init() completes and all panels are registered.
EVENT_DEF(EventStartupFinished);

/// Fired when the application is about to close.
EVENT_DEF(EventShutdown);

// ── WAD / ISO Events ──────────────────────────────────────────────────────

/// Fired after a WAD file has been loaded and parsed.
/// @param OpenWad* pointer to the newly opened WAD (valid for the WAD's lifetime)
EVENT_DEF(EventWadOpened, OpenWad*);

/// Fired when a WAD file is about to be closed.
/// @param size_t index of the WAD being closed in AssetDatabase::wads
EVENT_DEF(EventWadClosed, size_t);

/// Fired after an ISO PAK has been loaded.
/// @param OpenWad* pointer to the newly opened PAK
EVENT_DEF(EventPakOpened, OpenWad*);

/// Fired when all WADs and PAKs are closed.
EVENT_DEF(EventAllClosed);

// ── Asset Selection & Loading ─────────────────────────────────────────────

/// Fired when the user selects an asset in any browser panel.
/// @param ParsedEntry* the selected entry (can be nullptr for deselection)
/// @param OpenWad*     the parent WAD/PAK containing the entry
EVENT_DEF(EventAssetSelected, ParsedEntry*, OpenWad*);

/// Fired after an asset's node data has been loaded via EnsureNodeData.
/// @param ParsedEntry* the entry whose data is now available
EVENT_DEF(EventAssetLoaded, ParsedEntry*);

/// Fired when a new document/viewer tab is opened.
/// @param IDocumentContent* the opened document
EVENT_DEF(EventDocumentOpened, GOW::IDocumentContent*);

/// Fired when animation data is loaded into a scene (e.g. Viewport3D).
/// @param std::shared_ptr<GOW::AnimationData> the loaded animation data
EVENT_DEF(EventAnimationLoaded, std::shared_ptr<GOW::AnimationData>);

// ── UI State Events ───────────────────────────────────────────────────────

/// Per-frame tick — subscribers that need continuous updates (animations, progress).
/// Marked NO_LOG to avoid spamming the debug output.
EVENT_DEF_NO_LOG(EventFrameTick);

/// Fired when the AppConfig is modified (settings changed).
EVENT_DEF(EventConfigChanged);
