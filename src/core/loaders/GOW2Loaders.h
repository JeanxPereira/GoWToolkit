#pragma once
#include "core/loaders/IAssetLoader.h"

namespace GOW {

class GOW2ModelLoader : public IAssetLoader {
public:
    bool canHandle(const std::string& schemaType) const override;
    std::shared_ptr<IDocumentContent> load(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOW2MeshLoader : public IAssetLoader {
public:
    bool canHandle(const std::string& schemaType) const override;
    std::shared_ptr<IDocumentContent> load(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOW2TextureLoader : public IAssetLoader {
public:
    bool canHandle(const std::string& schemaType) const override;
    std::shared_ptr<IDocumentContent> load(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOW2MaterialLoader : public IAssetLoader {
public:
    bool canHandle(const std::string& schemaType) const override;
    std::shared_ptr<IDocumentContent> load(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOW2SoundLoader : public IAssetLoader {
public:
    bool canHandle(const std::string& schemaType) const override;
    std::shared_ptr<IDocumentContent> load(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOW2VagLoader : public IAssetLoader {
public:
    bool canHandle(const std::string& schemaType) const override;
    std::shared_ptr<IDocumentContent> load(const ParsedEntry& entry, OpenWad& wad) override;
};

class GOW2VpkLoader : public IAssetLoader {
public:
    bool canHandle(const std::string& schemaType) const override;
    std::shared_ptr<IDocumentContent> load(const ParsedEntry& entry, OpenWad& wad) override;
};

} // namespace GOW
