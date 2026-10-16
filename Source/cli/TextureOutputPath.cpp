#include "TextureOutputPath.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace Onyx::Cli {

bool IsSafeTextureOutputFileName(std::string_view fileName) {
    if (fileName.empty() || fileName.find("..") != std::string_view::npos ||
        fileName.find('/') != std::string_view::npos ||
        fileName.find('\\') != std::string_view::npos) {
        return false;
    }

    const size_t dot = fileName.rfind('.');
    if (dot == std::string_view::npos) return false;

    std::string extension(fileName.substr(dot));
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".png";
}

} // namespace Onyx::Cli
