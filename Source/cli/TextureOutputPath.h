#pragma once

#include <string_view>

namespace Onyx::Cli {

inline constexpr std::string_view kTextureOutputFileName = "texture.png";

// Texture exports use a fixed filename in the process working directory. This
// keeps command-line input out of the filesystem path entirely.
bool IsSafeTextureOutputFileName(std::string_view fileName);

} // namespace Onyx::Cli
