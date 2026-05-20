#pragma once

// ── Legacy umbrella header ────────────────────────────────────────────────
//
// `WadTypes.h` historically held `WadAssetName`, `WadEntryRole`,
// `WadBlock`, `ParsedEntry`, `OpenWad`, and type-string helpers. M1.T1
// split each concept into its own header under `core/domain/`. This
// umbrella keeps existing call sites working (strangler-fig
// migration); new code should `#include "core/domain/<Specific>.h"`
// instead of `core/WadTypes.h`.
//
// Will be deleted entirely in M4.T5.

#include "core/domain/Entry.h"
#include "core/domain/Wad.h"
#include "core/domain/WadEntryRoleLegacy.h"
#include "core/types/GameVersion.h"
#include "core/types/TypeId.h"
