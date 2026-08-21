#pragma once

// ── Legacy umbrella header ────────────────────────────────────────────────
//
// `WadTypes.h` historically held `WadAssetName`, `WadEntryRole`,
// `WadBlock`, `AssetEntry`, `AssetContainer`, and type-string helpers. M1.T1
// split each concept into its own header under `core/domain/`. This
// umbrella keeps existing call sites working (strangler-fig
// migration); new code should `#include "core/domain/<Specific>.h"`
// instead of `core/WadTypes.h`.
//
// Will be deleted entirely in M4.T5.

#include <Onyx/Domain/Entry.h>
#include <Onyx/Domain/Wad.h>
#include "core/domain/WadEntryRoleLegacy.h"
#include <Onyx/Types/TypeId.h>

// Onyx v1.0.0 removed the global-scope aliases for these two (its audit gap G5).
// The app reaches them through this umbrella header in 245 places across 25
// files, so they are re-introduced here rather than qualified at every site --
// a mechanical sweep of that size would bury the port's real changes.
// Deleting WadTypes.h entirely is a separate, already-planned piece of work.
using AssetEntry     = Onyx::Domain::AssetEntry;
using AssetContainer = Onyx::Domain::AssetContainer;
