#pragma once
#include <Onyx/Domain/IAssetProfile.h>
#include <Onyx/Schema/StructDef.h>
#include <map>
#include <string>
#include <vector>

namespace Onyx {

class ProfileGOWR : public Domain::IAssetProfile {
public:
    ProfileGOWR();
    ~ProfileGOWR() override = default;

    std::string GetName() const override { return "God of War Ragnarok (PS4/PS5)"; }
    std::vector<std::string> GetHints() const override { return {"gowr", "ragnarok", "ps4", "ps5"}; }

    Onyx::Domain::OpenFilter GetOpenFilter() const override {
        return {"God of War (2018) / Ragnarok", {"wad"}};
    }

    bool Detect(const std::filesystem::path& path) const override;
    
    std::shared_ptr<Vfs::IVirtualFileSystem> MountArchive(const std::filesystem::path& path) override;

    bool ParseContainer(std::shared_ptr<Vfs::IFile> file, AssetContainer& outWad) override;

    bool LoadFromArchive(std::shared_ptr<Vfs::IVirtualFileSystem> vfs, AssetContainer& outWad) override;

    bool IsContainerEntry(const AssetEntry& entry) const override;

    void PrepareForParse(const std::filesystem::path& path) override;
private:
    std::map<std::string, std::shared_ptr<Schema::StructDef>> m_schemas;
};

} // namespace Onyx
