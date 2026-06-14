#pragma once
#include <Onyx/Domain/Wad.h>
#include <Onyx/Types/GameVersion.h>
#include <Onyx/Types/TypeId.h>
#include <Onyx/Types/TypeRegistry.h>
#include <memory>
#include <string>

#include <Onyx/Domain/MediaKind.h>
#include <functional>
#include <unordered_map>

namespace Onyx::Viewers { class IDocumentContent; }

namespace Onyx::App {

class ViewerRegistry {
public:
    using Factory = std::function<std::shared_ptr<Viewers::IDocumentContent>(const AssetEntry&, AssetContainer&)>;

    ViewerRegistry();

    bool CanHandle(Types::TypeId typeId) const;
    std::shared_ptr<Viewers::IDocumentContent> Open(const AssetEntry& entry, AssetContainer& wad) const;

private:
    std::unordered_map<Domain::MediaKind, Factory> m_factories;
};

} // namespace Onyx::App
