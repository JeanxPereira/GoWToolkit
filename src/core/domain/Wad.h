#pragma once

// `OpenWad` aggregates everything an opened WAD exposes to the rest of
// the toolkit: source filename, file handle, owning profile, and the
// flat list of parsed entries. M1.T1 split this out of the legacy
// `core/WadTypes.h` umbrella so consumers can depend on `Wad.h`
// directly without pulling the rest of the surface.

#include <memory>
#include <string>
#include <vector>

#include "core/domain/Entry.h"

namespace GOW {
    class IGameProfile;
    class IFile;
}

// NOTE: `OpenWad` lives at global scope to match the legacy layout in
// `core/WadTypes.h`. The fields it owns are scoped under `GOW::`. It
// will move into the namespace in a later milestone.
struct OpenWad {
    std::string                        filename;
    std::string                        fullPath;
    std::shared_ptr<GOW::IGameProfile> profile;
    std::shared_ptr<GOW::IFile>        fileSource;
    std::vector<ParsedEntry>           entries;
};
