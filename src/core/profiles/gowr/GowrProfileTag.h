#pragma once
#include "core/profiles/gowr/GowrTaxonomy.h"

namespace Onyx {
namespace Gowr {

struct GowrProfileTag {
    WadEntryRole role       = WadEntryRole::Unknown;
    WadBlock     block      = WadBlock::Unknown;
    WadAssetName parsedName;
};

} // namespace Gowr
} // namespace Onyx
