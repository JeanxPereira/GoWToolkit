#pragma once

#include "core/WadTypes.h"
#include <string>

namespace GOW {
class IFile;

class ScriptTargetParser {
public:
    /// Parses a Script entry payload (magic 0x00010004) and extracts the TargetName.
    /// Returns the TargetName string (e.g. "SCR_Sky"), or empty string on failure.
    static std::string ExtractTargetName(const ParsedEntry& entry, std::shared_ptr<IFile> file);
};

} // namespace GOW
