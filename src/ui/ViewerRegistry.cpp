#include "ui/ViewerRegistry.h"
#include "core/Logger.h"

namespace GOW {

void ViewerRegistry::RegisterLoader(std::unique_ptr<IAssetLoader> loader) {
    if (loader)
        m_loaders.push_back(std::move(loader));
}

bool ViewerRegistry::CanHandle(const std::string& schemaType) const {
    for (const auto& loader : m_loaders) {
        if (loader->canHandle(schemaType))
            return true;
    }
    return false;
}

std::shared_ptr<IDocumentContent> ViewerRegistry::Open(const ParsedEntry& entry, OpenWad& wad) const {
    for (const auto& loader : m_loaders) {
        if (loader->canHandle(entry.schemaType)) {
            auto viewer = loader->load(entry, wad);
            if (!viewer) {
                LOG_ERR("[ViewerRegistry] Loader for '%s' returned null for entry '%s'",
                        entry.schemaType.c_str(), entry.name.c_str());
            }
            return viewer;
        }
    }
    return nullptr;
}

} // namespace GOW
