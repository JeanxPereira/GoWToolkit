#pragma once

// â”€â”€ Legacy umbrella header â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
