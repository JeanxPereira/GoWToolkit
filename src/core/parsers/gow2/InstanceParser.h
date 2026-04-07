#pragma once

#include <string>
#include <vector>
#include <memory>
#include "core/WadTypes.h"
#include "core/vfs/IFile.h"

namespace GOW {

struct InstanceData {
    // GOW1: extracted from binary data [0x04:0x1C].
    // GOW2: empty (Object is found via child tree).
    std::string objectName;

    uint16_t id = 0;
    uint16_t params = 0;

    // Computed 4x4 transformation matrix (column-major, OpenGL convention).
    // For GOW1: built from Position1 (translation) + Rotation (euler XYZ + scale in W).
    // For GOW2: built from 3x3 orientation matrix + Vec3 position.
    float transformMatrix[16];

    // True if this instance looks like a sky dome (heuristic: name contains "sky")
    bool isSky = false;
};

class GOW2InstanceParser {
public:
    static std::shared_ptr<InstanceData> Parse(const ParsedEntry& entry, std::shared_ptr<IFile> parentFile);
};

} // namespace GOW
