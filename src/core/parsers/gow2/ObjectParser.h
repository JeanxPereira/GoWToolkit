#pragma once
#include "core/parsers/shared/ObjectData.h"
#include <memory>
#include <cstdint>

namespace GOW {

class GOW2ObjectParser {
public:
    /// Parse an Object payload (magic 0x00010001 for GOW2).
    /// Returns nullptr on failure.
    static std::unique_ptr<ObjectData> Parse(
        const uint8_t* data, uint32_t size, uint32_t magic);

private:
    static std::unique_ptr<ObjectData> ParseGOW2(const uint8_t* data, uint32_t size);

    /// Compute ParentToJoint and BindToJointMat from raw matrices.
    /// Equivalent to obj.go FillJoints()
    static void FillJoints(ObjectData& obj);
};

} // namespace GOW
