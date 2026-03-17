#pragma once
#include "core/loaders/IAssetLoader.h"

namespace GOW {

class GOWRMeshLoader : public IAssetLoader {
public:
    bool canHandle(const std::string& schemaType) const override;
    std::shared_ptr<IDocumentContent> load(const ParsedEntry& entry, OpenWad& wad) override;
};

} // namespace GOW
