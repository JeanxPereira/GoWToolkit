#include "ScriptTargetParser.h"
#include "core/vfs/IFile.h"
#include "core/Logger.h"
#include <vector>

namespace Onyx {

std::string ScriptTargetParser::ExtractTargetName(const AssetEntry& entry, std::shared_ptr<IFile> file) {
    if (!file || !file->IsValid() || entry.size < 0x14) return "";

    // The script header is 0x24 bytes long.
    // Magic is at 0x0 (4 bytes): 0x00010004
    // TargetName is at 0x4 (16 bytes): null-padded string
    
    file->Seek(entry.offset + 4, SEEK_SET);
    char buf[16] = {0};
    if (file->Read(buf, 16) != 16) {
        return "";
    }

    // Ensure null termination for parsing
    return std::string(buf, strnlen(buf, 16));
}

} // namespace Onyx
