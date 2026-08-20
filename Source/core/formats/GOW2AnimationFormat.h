#pragma once
#include <Onyx/Schema/AssetFormat.h>

namespace Onyx {

// â”€â”€ GOW2 Animation Format â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Magic: 0x00000003
class GOW2AnimationFormat : public Schema::AssetFormat {
protected:
    void Build() override {
        Struct("GOW2Animation", 32, // Minimal header mapping
            Key("magic", 0x0),
            UInt16("boneCount", 0x4), // typically at 0x4
            UInt16("frameCount", 0x6),
            Float("duration", 0x8),
            Hex("animData", 0xC, 20)
        );
    }
};

} // namespace Onyx
