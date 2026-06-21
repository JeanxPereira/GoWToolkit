#pragma once
#include <Onyx/Domain/IAssetProfile.h>
#include <Onyx/Schema/StructDef.h>
#include <map>
#include <string>
#include <vector>

namespace Onyx {

class ProfileGOW2 : public Domain::IAssetProfile {
public:
    ProfileGOW2();
    ~ProfileGOW2() override = default;

    std::string GetName() const override { return "God of War II (PS2)"; }
    std::vector<std::string> GetHints() const override { return {"gow1", "gow2", "ps2"}; }

    Onyx::Domain::OpenFilter GetOpenFilter() const override {
        return {"God of War I / II (PS2)", {"iso", "wad"}};
    }

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
