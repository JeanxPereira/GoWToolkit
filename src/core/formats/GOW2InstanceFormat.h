#pragma once
#include "core/schema/AssetFormat.h"

namespace GOW {

// ── GOW2 Instance Format ───────────────────────────────────────────────────
// Magic: 0x00030001
class GOW2InstanceFormat : public AssetFormat {
protected:
    void Build() override {
        Struct("GOW2Instance", 76,
            Hex("padding", 0x00, 28),
            UInt16("id", 0x1C),
            UInt16("params", 0x1E),
            Vec4("unkVec1", 0x20),
            Vec4("unkVec2", 0x30),
            Vec4("unkVec3", 0x40),
            Vec3("position", 0x50),
            UInt("unk0", 0x5C),
            UInt("unk1", 0x60),
            UInt("unk2", 0x64)
        );
    }
};

} // namespace GOW
