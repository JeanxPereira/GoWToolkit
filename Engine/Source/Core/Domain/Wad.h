#pragma once

// `AssetContainer` aggregates everything an opened WAD exposes to the rest of
// the toolkit: source filename, file handle, owning profile, and the
// flat list of parsed entries. M1.T1 split this out of the legacy
// `core/WadTypes.h` umbrella so consumers can depend on `Wad.h`
// directly without pulling the rest of the surface.

#include <memory>
#include <string>
#include <vector>

#include "Core/Domain/Entry.h"

namespace Onyx::Domain {
    class IAssetProfile;
}
namespace Onyx::Vfs {
    class IFile;
}

namespace Onyx::Domain {

struct AssetContainer {
    std::string                        filename;
    std::string                        fullPath;
    std::shared_ptr<Onyx::Domain::IAssetProfile> profile;
    std::shared_ptr<Onyx::Vfs::IFile>   fileSource;
    std::vector<AssetEntry>           entries;
};

} // namespace Onyx::Domain

// Backwards-compat alias — keep existing call sites compiling at global scope.
using AssetContainer = Onyx::Domain::AssetContainer;
