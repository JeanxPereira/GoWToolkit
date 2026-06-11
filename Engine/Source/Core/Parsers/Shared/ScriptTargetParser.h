#pragma once

#include "Core/Domain/Wad.h"
#include "Core/Types/GameVersion.h"
#include "Core/Types/TypeId.h"
#include <string>

namespace Onyx::Vfs { class IFile; }

namespace Onyx::Parsers {

class ScriptTargetParser {
public:
    /// Parses a Script entry payload (magic 0x00010004) and extracts the TargetName.
    /// Returns the TargetName string (e.g. "SCR_Sky"), or empty string on failure.
    static std::string ExtractTargetName(const AssetEntry& entry, std::shared_ptr<Vfs::IFile> file);
};

} // namespace Onyx::Parsers

// Backwards-compat alias
namespace Onyx { using ScriptTargetParser = Parsers::ScriptTargetParser; } // namespace Onyx
