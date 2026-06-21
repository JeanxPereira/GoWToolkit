#pragma once
#include <Onyx/Schema/AssetFormat.h>

namespace Onyx {

// â”€â”€ GOW2 Model Format â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Used to map the headers of mdl_ files into the InfoTab properties grid.
// Magic: 0x000F (typically 0x0002000F or 0x0001000F)
class GOW2ModelFormat : public Schema::AssetFormat {
protected:
    void Build() override {
        Struct("GOW2Model", 24, // 0x18 header size
            Key("magic", 0x0),
            UInt("mdlCommentStart", 0x4),
            UInt16("partsCount", 0x8),
            Hex("unk0A", 0xA, 14), // padding until parts offset array
            Array("partOffsets", Schema::DataType::UInt32, 0x18, 0x8)
        );
    }
};

} // namespace Onyx
