#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>
#include "core/vfs/IFile.h"
#include "core/../rendering/GpuMesh.h"

namespace GOW {

class GOWRMeshParser {
public:
    // Parses a single MESH_DEFN file containing multiple mesh sub-definitions
    static bool ParseMeshDefn(std::shared_ptr<IFile> defFile, std::shared_ptr<IFile> lodpackFile, std::vector<std::shared_ptr<GpuMesh>>& outMeshes);
};

} // namespace GOW
