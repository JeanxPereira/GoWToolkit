#pragma once
#include <Onyx/Domain/IAssetProfile.h>
#include <vector>
#include <memory>
#include <filesystem>

namespace Onyx::Services {

class ProfileManager {
public:
    static ProfileManager& Get() {
        static ProfileManager instance;
        return instance;
    }

    void RegisterProfile(std::shared_ptr<Domain::IAssetProfile> profile);

    // Tenta achar um perfil compatÃ­vel iterando sobre o Detect de todos
    std::shared_ptr<Domain::IAssetProfile> DetectProfileForFile(const std::filesystem::path& path) const;

    // Busca por hint de nome (ex: "gowr", "gow2")
    std::shared_ptr<Domain::IAssetProfile> FindProfileByHint(const std::string& hint) const;

    const std::vector<std::shared_ptr<Domain::IAssetProfile>>& GetProfiles() const { return m_profiles; }

private:
    ProfileManager() = default;
    
    std::vector<std::shared_ptr<Domain::IAssetProfile>> m_profiles;
};

} // namespace Onyx::Services
