#include "ui/ViewerRegistry.h"
#include "core/Logger.h"

namespace GOW {

ViewerRegistry::ViewerRegistry() {
    auto defaultLegacyFactory = [](const ParsedEntry& entry, OpenWad& wad) {
        auto* handler = TypeRegistry::Get().Resolve(entry.typeId);
        if (handler) {
            return handler->CreateViewer(entry, wad);
        }
        return std::shared_ptr<IDocumentContent>(nullptr);
    };

    m_factories[MediaKind::Image] = defaultLegacyFactory;
    m_factories[MediaKind::Mesh] = defaultLegacyFactory;
    m_factories[MediaKind::Audio] = defaultLegacyFactory;
    m_factories[MediaKind::Video] = defaultLegacyFactory;
    m_factories[MediaKind::Material] = defaultLegacyFactory;
}

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

std::shared_ptr<IDocumentContent> ViewerRegistry::OpenByKind(const ParsedEntry& entry, OpenWad& wad) const {
    if (entry.kind == MediaKind::Unknown) {
        return nullptr;
    }
    
    auto it = m_factories.find(entry.kind);
    if (it != m_factories.end() && it->second) {
        auto viewer = it->second(entry, wad);
        if (!viewer) {
            LOG_DEBUG("[ViewerRegistry] Factory for kind %d returned null viewer for entry '%s'",
                      (int)entry.kind, entry.name.c_str());
        }
        return viewer;
    }
    
    return nullptr;
}

} // namespace GOW
