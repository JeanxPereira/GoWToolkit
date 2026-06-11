#pragma once
#include "Core/Parsers/Shared/ObjectData.h"
#include "Core/Vfs/IFile.h"
#include <memory>

namespace Onyx {

// Parser for GOWR goProto* files (skeleton / rig).
//
// Port of GoWRknk.cs:210-285. Produces an Parsers::ObjectData with one Joint per bone,
// parent indices, local TRS as parentToJoint, world rest pose as renderMat, and
// the inverse of renderMat as bindToJointMat for skinning.
class GOWRProtoParser {
public:
    static std::shared_ptr<Parsers::ObjectData> Parse(std::shared_ptr<Vfs::IFile> file);
};

} // namespace Onyx
