#pragma once
#include "core/domain/Wad.h"
#include "core/types/GameVersion.h"
#include "core/types/TypeId.h"
#include "core/types/TypeRegistry.h"
#include <memory>
#include <string>

#include "core/domain/MediaKind.h"
#include <functional>
#include <unordered_map>

namespace Onyx {

class ViewerRegistry {
public:
    using Factory = std::function<std::shared_ptr<IDocumentContent>(const AssetEntry&, AssetContainer&)>;

    ViewerRegistry();

    bool CanHandle(TypeId typeId) const;
    std::shared_ptr<IDocumentContent> Open(const AssetEntry& entry, AssetContainer& wad) const;

private:
    std::unordered_map<MediaKind, Factory> m_factories;
};

} // namespace Onyx
