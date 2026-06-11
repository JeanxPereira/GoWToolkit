#pragma once

#include "core/domain/Wad.h"
#include "core/types/GameVersion.h"
#include "core/types/TypeId.h"
#include <string>

namespace Onyx {
class IFile;

class ScriptTargetParser {
public:
    /// Parses a Script entry payload (magic 0x00010004) and extracts the TargetName.
    /// Returns the TargetName string (e.g. "SCR_Sky"), or empty string on failure.
    static std::string ExtractTargetName(const AssetEntry& entry, std::shared_ptr<IFile> file);
};

} // namespace Onyx
