#pragma once
#include <Onyx/Schema/AssetFormat.h>

namespace Onyx {

// â”€â”€ GOWR Mesh Definition â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class GOWRMeshDefnFormat : public Schema::AssetFormat {
protected:
    void Build() override {
        Struct("GOWRMeshDefn", 64, // minimal coverage of main header
            Key("magic", 0x0),
            UInt16("submeshCount", 0x10),
            Array("submeshOffsets", Schema::DataType::UInt32, 0x40, 0x10)
        );

        Struct("SubmeshHeader", 0x84,
            Vec3("extent", 0x10),
            Vec3("origin", 0x1C),
            UInt("gpuIndexOffset", 0x30),
            UInt("vertCount", 0x44),
            UInt("faceCount", 0x48),
            UInt("indCount", 0x5C),
            UInt("componentOffset", 0x60),
            UInt("bufOffsetsOffset", 0x64),
            UInt64("meshHash", 0x68, Schema::DisplayHint::Hex),
            UInt8("bufferCount", 0x80),
            UInt8("indicesStride", 0x81),
            UInt8("bytesPerVertex", 0x82),
            UInt8("componentCount", 0x83)
        );
    }
};

} // namespace Onyx
