#pragma once
#include "core/profiles/gowr/GowrTaxonomy.h"

namespace GOW {
namespace Gowr {

struct GowrProfileTag {
    WadEntryRole role       = WadEntryRole::Unknown;
    WadBlock     block      = WadBlock::Unknown;
    WadAssetName parsedName;
};

} // namespace Gowr
} // namespace GOW
