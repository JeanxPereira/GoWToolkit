#pragma once
#include "core/loaders/IAssetLoader.h"
#include <vector>
#include <memory>
#include <string>

namespace GOW {

class ViewerRegistry {
public:
    void RegisterLoader(std::unique_ptr<IAssetLoader> loader);
    bool CanHandle(const std::string& schemaType) const;
    std::shared_ptr<IDocumentContent> Open(const ParsedEntry& entry, OpenWad& wad) const;

private:
    std::vector<std::unique_ptr<IAssetLoader>> m_loaders;
};

} // namespace GOW
