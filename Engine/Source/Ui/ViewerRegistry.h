#pragma once
#include "Core/Domain/Wad.h"
#include "Core/Types/GameVersion.h"
#include "Core/Types/TypeId.h"
#include "Core/Types/TypeRegistry.h"
#include <memory>
#include <string>

#include "Core/Domain/MediaKind.h"
#include <functional>
#include <unordered_map>

namespace Onyx {

class ViewerRegistry {
public:
    using Factory = std::function<std::shared_ptr<IDocumentContent>(const AssetEntry&, AssetContainer&)>;

    ViewerRegistry();

    bool CanHandle(Types::TypeId typeId) const;
    std::shared_ptr<IDocumentContent> Open(const AssetEntry& entry, AssetContainer& wad) const;

private:
    std::unordered_map<Domain::MediaKind, Factory> m_factories;
};

} // namespace Onyx
