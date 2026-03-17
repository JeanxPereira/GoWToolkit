#pragma once
#include "core/WadTypes.h"
#include <vector>
#include <string>
#include <memory>

namespace GOW {

class GOW2MaterialParser {
public:
    struct MaterialLayer {
        std::string textureName;
        bool hasTexture = false;
    };

    struct MaterialData {
        std::vector<MaterialLayer> layers;
    };

    // Parses a MAT tag to extract its layers (and the texture names they reference)
    static std::unique_ptr<MaterialData> Parse(const ParsedEntry& matEntry, const std::shared_ptr<IFile>& fileSource);
};

} // namespace GOW
