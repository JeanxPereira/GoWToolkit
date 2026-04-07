#pragma once
#include "core/schema/AssetFormat.h"

namespace GOW {

// ── GOW2 Model Format ──────────────────────────────────────────────────────
// Used to map the headers of mdl_ files into the InfoTab properties grid.
// Magic: 0x000F (typically 0x0002000F or 0x0001000F)
class GOW2ModelFormat : public AssetFormat {
protected:
    void Build() override {
        Struct("GOW2Model", 24, // 0x18 header size
            Key("magic", 0x0),
            UInt("mdlCommentStart", 0x4),
            UInt16("partsCount", 0x8),
            Hex("unk0A", 0xA, 14), // padding until parts offset array
            Array("partOffsets", DataType::UInt32, 0x18, 0x8)
        );
    }
};

} // namespace GOW
