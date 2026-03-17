#pragma once
#include "../../interfaces/IGameProfile.h"
#include "../../schema/StructDef.h"
#include <map>

namespace GOW {

class ProfileGOWR : public IGameProfile {
public:
    ProfileGOWR();
    ~ProfileGOWR() override = default;

    std::string GetName() const override { return "God of War Ragnarok (PS4/PS5)"; }

    bool Detect(const std::filesystem::path& path) const override;
    
    std::shared_ptr<IVirtualFileSystem> MountArchive(const std::filesystem::path& path) override;
    
    bool ParseWad(std::shared_ptr<IFile> file, OpenWad& outWad) override;
    
    bool LoadFromArchive(std::shared_ptr<IVirtualFileSystem> vfs, OpenWad& outWad) override;
    
    std::shared_ptr<NodeInstance> CreateNodeInstance(const std::string& typeName, std::shared_ptr<IFile> fileData) override;

private:
    std::map<std::string, std::shared_ptr<StructDef>> m_schemas;
};

} // namespace GOW
