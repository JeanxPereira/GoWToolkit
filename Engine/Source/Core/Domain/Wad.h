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

namespace Onyx {
    class IAssetProfile;
    class IFile;
}

// NOTE: `AssetContainer` lives at global scope to match the legacy layout in
// `core/WadTypes.h`. The fields it owns are scoped under `Onyx::`. It
// will move into the namespace in a later milestone.
struct AssetContainer {
    std::string                        filename;
    std::string                        fullPath;
    std::shared_ptr<Onyx::IAssetProfile> profile;
    std::shared_ptr<Onyx::IFile>        fileSource;
    std::vector<AssetEntry>           entries;
};
