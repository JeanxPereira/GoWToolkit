#pragma once
#include <Onyx/Parsers/ObjectData.h>
#include <memory>
#include <cstdint>

namespace Onyx {

class GOW2ObjectParser {
public:
    /// Parse an Object payload (magic 0x00010001 for GOW2).
    /// Returns nullptr on failure.
    static std::unique_ptr<Parsers::ObjectData> Parse(
        const uint8_t* data, uint32_t size, uint32_t magic);

private:
    static std::unique_ptr<Parsers::ObjectData> ParseGOW2(const uint8_t* data, uint32_t size);

    /// Compute ParentToJoint and BindToJointMat from raw matrices.
    /// Equivalent to obj.go FillJoints()
    static void FillJoints(Parsers::ObjectData& obj);
};

} // namespace Onyx
