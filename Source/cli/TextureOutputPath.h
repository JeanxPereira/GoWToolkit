#pragma once

#include <string_view>

namespace Onyx::Cli {

// Texture exports are deliberately confined to the process working directory.
// The CLI accepts a leaf PNG filename, never an absolute or relative path.
bool IsSafeTextureOutputFileName(std::string_view fileName);

} // namespace Onyx::Cli
