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
        
        float blendColor[4];
        float floatUnk;
        
        int renderingMethod;
        bool filterLinear;
        bool disableDepthWrite;
        
        bool uvAnimEnabled;
        bool colorAnimEnabled;
    };

    struct MaterialData {
        float baseColor[4];
        std::vector<MaterialLayer> layers;
    };

    static std::unique_ptr<MaterialData> Parse(const ParsedEntry& matEntry, const std::shared_ptr<IFile>& fileSource);
};

} // namespace GOW