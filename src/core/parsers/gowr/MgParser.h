#pragma once
#include "core/vfs/IFile.h"
#include <memory>
#include <vector>
#include <cstdint>

namespace GOW {

// Parses an MG_*.bin (mesh-group / bone-binding) file.
//
// Each MG-def groups one or more MESH submeshes that all rigidly belong to one
// skeleton bone. This parser collapses that into a flat map:
//   meshSubmeshIdx → parentBoneIdx
//
// Port of GOWTool MG::Parse (Formats.cpp:165-220).
class GOWRMgParser {
public:
    // outParentBone[meshSubmeshIdx] = bone index that owns that submesh, or
    // 0xFFFF if the submesh wasn't referenced by any MG-def.
    // meshSubmeshCount sets the output vector size (= number of MESH submeshes).
    static bool Parse(std::shared_ptr<IFile> mgFile,
                      uint32_t meshSubmeshCount,
                      std::vector<uint16_t>& outParentBone);
};

} // namespace GOW
