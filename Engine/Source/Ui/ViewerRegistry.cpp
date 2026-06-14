#include "Ui/ViewerRegistry.h"
#include "Core/Logger.h"

namespace Onyx::App {

ViewerRegistry::ViewerRegistry() {
    auto defaultLegacyFactory = [](const AssetEntry& entry, AssetContainer& wad) {
        auto* handler = Types::TypeRegistry::Get().Resolve(entry.typeId);
        if (handler) {
            return handler->CreateViewer(entry, wad);
        }
        return std::shared_ptr<Viewers::IDocumentContent>(nullptr);
    };

    m_factories[Domain::MediaKind::Image] = defaultLegacyFactory;
    m_factories[Domain::MediaKind::Mesh] = defaultLegacyFactory;
    m_factories[Domain::MediaKind::Audio] = defaultLegacyFactory;
    m_factories[Domain::MediaKind::Video] = defaultLegacyFactory;
    m_factories[Domain::MediaKind::Material] = defaultLegacyFactory;
}

bool ViewerRegistry::CanHandle(Types::TypeId typeId) const {
    return Types::TypeRegistry::Get().Resolve(typeId) != nullptr;
}

std::shared_ptr<Viewers::IDocumentContent> ViewerRegistry::Open(const AssetEntry& entry, AssetContainer& wad) const {
    // 1. Try TypeRegistry handler first (preferred path)
    auto* handler = Types::TypeRegistry::Get().Resolve(entry.typeId);
    if (handler) {
        auto viewer = handler->CreateViewer(entry, wad);
        if (!viewer) {
            LOG_DEBUG("[ViewerRegistry] Handler for '%s' returned null viewer for entry '%s'",
                      handler->GetName(), entry.name.c_str());
        }
        return viewer;
    }
    
    // 2. Fallback: try kind-based factory
    if (entry.kind != Domain::MediaKind::Unknown) {
        auto it = m_factories.find(entry.kind);
        if (it != m_factories.end() && it->second) {
            auto viewer = it->second(entry, wad);
            if (viewer) return viewer;
            LOG_DEBUG("[ViewerRegistry] Factory for kind %d returned null viewer for entry '%s'",
                      (int)entry.kind, entry.name.c_str());
        }
    }
    
    LOG_WARN("ViewerRegistry: No viewer found for TypeId=%d, kind=%d",
            (int)entry.typeId.value, (int)entry.kind);
    return nullptr;
}

} // namespace Onyx::App
