#pragma once

// Legacy classification enums used by the GOWR `WadNodeBuilder` and by the
// UI to group entries by semantic role / functional block.
//
// These will be retired in M4 once the structural rebuild lands. Until
// then they're versioned in their own header so M1's strangler-fig split
// of `WadTypes.h` doesn't drag the much larger `Entry.h` into every TU
// that only needs the enums.

#pragma once

#include "core/profiles/gowr/GowrTaxonomy.h"

// Legacy aliases. Will be retired.
using WadEntryRole [[deprecated("use profiles/gowr/GowrTaxonomy.h")]] = GOW::Gowr::WadEntryRole;
using WadBlock     [[deprecated("use profiles/gowr/GowrTaxonomy.h")]] = GOW::Gowr::WadBlock;
