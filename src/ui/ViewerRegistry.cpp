#include "ui/ViewerRegistry.h"
#include "core/Logger.h"

namespace GOW {

bool ViewerRegistry::CanHandle(GameVersion ver, TypeId typeId, const std::string& schemaType) const {
    auto* handler = TypeRegistry::Get().Resolve(typeId);
    if (handler) {
        return true; 
    }
    
    // GOWR string-based fallback
    if (schemaType == "GOWR_TEXTURE") return true;
    if (schemaType == "GOWR_MESH_DEFN" || schemaType == "GOWR_MODL_DEFN") return true;
    
    return false;
}

std::shared_ptr<IDocumentContent> ViewerRegistry::Open(const ParsedEntry& entry, OpenWad& wad) const {
    auto* handler = TypeRegistry::Get().Resolve(entry.typeId);
    if (handler) {
        auto viewer = handler->CreateViewer(entry, wad);
        if (!viewer) {
            LOG_DEBUG("[ViewerRegistry] Handler for '%s' returned null viewer for entry '%s'",
                      handler->GetName(), entry.name.c_str());
        }
        return viewer;
    }
    
    LOG_ERR("[ViewerRegistry] No handler or fallback for typeId %d (schema: %s)", 
            (int)entry.typeId, entry.schemaType.c_str());
    return nullptr;
}

} // namespace GOW
