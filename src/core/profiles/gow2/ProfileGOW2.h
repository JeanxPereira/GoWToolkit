#pragma once
#include "../../interfaces/IGameProfile.h"
#include "../../schema/StructDef.h"
#include <map>

namespace Onyx {

class ProfileGOW2 : public IAssetProfile {
public:
    ProfileGOW2();
    ~ProfileGOW2() override = default;

    std::string GetName() const override { return "God of War II (PS2)"; }

    bool Detect(const std::filesystem::path& path) const override;
    
    std::shared_ptr<IVirtualFileSystem> MountArchive(const std::filesystem::path& path) override;
    
    bool ParseContainer(std::shared_ptr<IFile> file, AssetContainer& outWad) override;
    
    bool LoadFromArchive(std::shared_ptr<IVirtualFileSystem> vfs, AssetContainer& outWad) override;
    
private:
    void RegisterSchemas();
    bool LoadFromArchiveGOW2(std::shared_ptr<IVirtualFileSystem> vfs,
                              IFile* tocFile, AssetContainer& outWad);

    std::map<std::string, std::shared_ptr<StructDef>> m_schemas;
};

} // namespace Onyx
