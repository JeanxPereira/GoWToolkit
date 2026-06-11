#pragma once

// GOWToolkit Event Catalog
// All events used across the application.
// Convention: Event names match the subsystem they belong to.

#include "Core/EventManager.h"

// Forward declarations
#include <memory>
namespace Onyx::Domain { struct AssetContainer; struct AssetEntry; }
using AssetContainer = Onyx::Domain::AssetContainer;
using AssetEntry     = Onyx::Domain::AssetEntry;
namespace Onyx::Viewers { class IDocumentContent; }
namespace Onyx::Parsers { struct AnimationData; }
class AppConfig;

// ── Lifecycle Events ───────────────────────────────────────────────────────

/// Fired after App::init() completes and all panels are registered.
EVENT_DEF(EventStartupFinished);

/// Fired when the application is about to close.
EVENT_DEF(EventShutdown);

// ── WAD / ISO Events ──────────────────────────────────────────────────────

/// Fired after a WAD file has been loaded and parsed.
/// @param AssetContainer* pointer to the newly opened WAD (valid for the WAD's lifetime)
EVENT_DEF(EventWadOpened, AssetContainer*);

/// Fired when a WAD file is about to be closed.
/// @param size_t index of the WAD being closed in AssetDatabase::wads
EVENT_DEF(EventWadClosed, size_t);

/// Fired after an ISO PAK has been loaded.
/// @param AssetContainer* pointer to the newly opened PAK
EVENT_DEF(EventPakOpened, AssetContainer*);

/// Fired when all WADs and PAKs are closed.
EVENT_DEF(EventAllClosed);

// ── Asset Selection & Loading ─────────────────────────────────────────────

/// Fired when the user selects an asset in any browser panel.
/// @param AssetEntry* the selected entry (can be nullptr for deselection)
/// @param AssetContainer*     the parent WAD/PAK containing the entry
EVENT_DEF(EventAssetSelected, AssetEntry*, AssetContainer*);

/// Fired after an asset's node data has been loaded via EnsureNodeData.
/// @param AssetEntry* the entry whose data is now available
EVENT_DEF(EventAssetLoaded, AssetEntry*);

/// Fired when a new document/viewer tab is opened.
/// @param IDocumentContent* the opened document
EVENT_DEF(EventDocumentOpened, Onyx::Viewers::IDocumentContent*);

/// Fired when animation data is loaded into a scene (e.g. Viewport3D).
/// @param std::shared_ptr<Onyx::Parsers::AnimationData> the loaded animation data
EVENT_DEF(EventAnimationLoaded, std::shared_ptr<Onyx::Parsers::AnimationData>);

// ── UI State Events ───────────────────────────────────────────────────────

/// Per-frame tick — subscribers that need continuous updates (animations, progress).
/// Posted near the TOP of the frame, BEFORE panels/documents draw.
/// Marked NO_LOG to avoid spamming the debug output.
EVENT_DEF_NO_LOG(EventFrameTick);

/// End-of-frame tick — posted AFTER all panels and documents have drawn for the
/// frame. Subscribers that must observe per-frame UI mutations (e.g. mirroring a
/// widget value back to config the same frame the user changed it) belong here.
/// Marked NO_LOG to avoid spamming the debug output.
EVENT_DEF_NO_LOG(EventFrameEnd);

/// Fired when the AppConfig is modified (settings changed).
EVENT_DEF(EventConfigChanged);
