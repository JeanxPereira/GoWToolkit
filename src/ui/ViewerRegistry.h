#pragma once
#include "core/WadTypes.h"
#include "core/types/TypeRegistry.h"
#include <memory>
#include <string>

namespace GOW {

class ViewerRegistry {
public:
    bool CanHandle(GameVersion ver, TypeId typeId, const std::string& schemaType) const;
    std::shared_ptr<IDocumentContent> Open(const ParsedEntry& entry, OpenWad& wad) const;
};

} // namespace GOW
