#pragma once
#include "core/parsers/shared/ObjectData.h"
#include "core/vfs/IFile.h"
#include <memory>

namespace GOW {

// Parser for GOWR goProto* files (skeleton / rig).
//
// Port of GoWRknk.cs:210-285. Produces an ObjectData with one Joint per bone,
// parent indices, local TRS as parentToJoint, world rest pose as renderMat, and
// the inverse of renderMat as bindToJointMat for skinning.
class GOWRProtoParser {
public:
    static std::shared_ptr<ObjectData> Parse(std::shared_ptr<IFile> file);
};

} // namespace GOW
