#pragma once
#include "core/WadTypes.h"
#include "core/types/TypeRegistry.h"
#include <memory>
#include <string>

#include "core/domain/MediaKind.h"
#include <functional>
#include <unordered_map>

namespace GOW {

class ViewerRegistry {
public:
    using Factory = std::function<std::shared_ptr<IDocumentContent>(const ParsedEntry&, OpenWad&)>;

    ViewerRegistry();

    bool CanHandle(GameVersion ver, TypeId typeId, const std::string& schemaType) const;
    std::shared_ptr<IDocumentContent> Open(const ParsedEntry& entry, OpenWad& wad) const;
    std::shared_ptr<IDocumentContent> OpenByKind(const ParsedEntry& entry, OpenWad& wad) const;

private:
    std::unordered_map<MediaKind, Factory> m_factories;
};

} // namespace GOW
