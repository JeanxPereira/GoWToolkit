#pragma once
// NOTE (Phase 2 Task 3): this class used to implement `Domain::IAssetProfile`,
// which Onyx v1.1 deleted outright (no header, no replacement type). The
// production entry point for GOW1/2 is now `Onyx::Gow2::Gow2Module`
// (Source/core/modules/Gow2Module.{h,cpp}), which ports this class's parsing
// logic onto `IGameModule`. This class survives, stripped of the deleted
// base and its `override` specifiers, ONLY because `tests/golden_helpers.cpp`
// still constructs it directly and calls `ParseContainer(file, wad)` as a
// plain (non-virtual) method — rewiring that golden test onto Workspace/
// Gow2Module is Task 4's job, not this one's. Do not add new callers.
#include <Onyx/Schema/StructDef.h>
#include <Onyx/Vfs/IFile.h>
#include <Onyx/Vfs/IVirtualFileSystem.h>
#include "core/WadTypes.h"
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Onyx {

class ProfileGOW2 {
public:
    ProfileGOW2();
    ~ProfileGOW2() = default;

    std::string GetName() const { return "God of War II (PS2)"; }
    std::vector<std::string> GetHints() const { return {"gow1", "gow2", "ps2"}; }

    bool Detect(const std::filesystem::path& path) const;

    std::shared_ptr<Vfs::IVirtualFileSystem> MountArchive(const std::filesystem::path& path);

    bool ParseContainer(std::shared_ptr<Vfs::IFile> file, AssetContainer& outWad);

    bool LoadFromArchive(std::shared_ptr<Vfs::IVirtualFileSystem> vfs, AssetContainer& outWad);

    bool IsContainerEntry(const AssetEntry& entry) const;

private:
    void RegisterSchemas();
    bool LoadFromArchiveGOW2(std::shared_ptr<Vfs::IVirtualFileSystem> vfs,
                              Vfs::IFile* tocFile, AssetContainer& outWad);

    std::map<std::string, std::shared_ptr<Schema::StructDef>> m_schemas;
};

} // namespace Onyx
