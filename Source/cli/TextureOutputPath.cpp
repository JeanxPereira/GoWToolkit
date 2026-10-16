#include "TextureOutputPath.h"

namespace Onyx::Cli {

bool IsSafeTextureOutputFileName(std::string_view fileName) {
    return fileName == kTextureOutputFileName;
}

} // namespace Onyx::Cli
