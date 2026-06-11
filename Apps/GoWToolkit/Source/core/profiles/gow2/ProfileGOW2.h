#pragma once
#include "Core/Interfaces/IGameProfile.h"
#include "Core/Schema/StructDef.h"
#include <map>

namespace Onyx {

class ProfileGOW2 : public IAssetProfile {
public:
    ProfileGOW2();
    ~ProfileGOW2() override = default;

    std::string GetName() const override { return "God of War II (PS2)"; }

    bool Detect(const std::filesystem::path& path) const override;
    
    std::shared_ptr<Vfs::IVirtualFileSystem> MountArchive(const std::filesystem::path& path) override;

    bool ParseContainer(std::shared_ptr<Vfs::IFile> file, AssetContainer& outWad) override;

    bool LoadFromArchive(std::shared_ptr<Vfs::IVirtualFileSystem> vfs, AssetContainer& outWad) override;

    bool IsContainerEntry(const AssetEntry& entry) const override;

private:
    void RegisterSchemas();
    bool LoadFromArchiveGOW2(std::shared_ptr<Vfs::IVirtualFileSystem> vfs,
                              Vfs::IFile* tocFile, AssetContainer& outWad);

    std::map<std::string, std::shared_ptr<Schema::StructDef>> m_schemas;
};

} // namespace Onyx
