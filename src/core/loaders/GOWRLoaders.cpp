#include "core/loaders/GOWRLoaders.h"
#include "ui/viewers/Viewport3D.h"

namespace GOW {

bool GOWRMeshLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOWR_MESH_DEFN" || schemaType == "GOWR_MODEL_INSTANCE";
}

std::shared_ptr<IDocumentContent> GOWRMeshLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;
    auto vp = std::make_shared<GOW::Viewport3D>(entry.name);
    vp->AddTestCube();
    return vp;
}

} // namespace GOW
