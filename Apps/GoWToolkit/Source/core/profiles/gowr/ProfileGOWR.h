#pragma once
#include "Core/Interfaces/IGameProfile.h"
#include "Core/Schema/StructDef.h"
#include <map>

namespace Onyx {

class ProfileGOWR : public IAssetProfile {
public:
    ProfileGOWR();
    ~ProfileGOWR() override = default;

    std::string GetName() const override { return "God of War Ragnarok (PS4/PS5)"; }

    bool Detect(const std::filesystem::path& path) const override;
    
    std::shared_ptr<Vfs::IVirtualFileSystem> MountArchive(const std::filesystem::path& path) override;

    bool ParseContainer(std::shared_ptr<Vfs::IFile> file, AssetContainer& outWad) override;

    bool LoadFromArchive(std::shared_ptr<Vfs::IVirtualFileSystem> vfs, AssetContainer& outWad) override;

    bool IsContainerEntry(const AssetEntry& entry) const override;
private:
    std::map<std::string, std::shared_ptr<StructDef>> m_schemas;
};

} // namespace Onyx
