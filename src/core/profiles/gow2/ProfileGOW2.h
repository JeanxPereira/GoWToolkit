#pragma once
#include "../../interfaces/IGameProfile.h"
#include "../../schema/StructDef.h"
#include <map>

namespace GOW {

class ProfileGOW2 : public IGameProfile {
public:
    ProfileGOW2();
    ~ProfileGOW2() override = default;

    std::string GetName() const override { return "God of War II (PS2)"; }

    bool Detect(const std::filesystem::path& path) const override;
    
    std::shared_ptr<IVirtualFileSystem> MountArchive(const std::filesystem::path& path) override;
    
    bool ParseWad(std::shared_ptr<IFile> file, OpenWad& outWad) override;
    
    bool LoadFromArchive(std::shared_ptr<IVirtualFileSystem> vfs, OpenWad& outWad) override;
    
private:
    void RegisterSchemas();
    bool LoadFromArchiveGOW2(std::shared_ptr<IVirtualFileSystem> vfs,
                              IFile* tocFile, OpenWad& outWad);

    std::map<std::string, std::shared_ptr<StructDef>> m_schemas;
};

} // namespace GOW
