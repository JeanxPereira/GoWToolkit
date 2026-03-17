#pragma once
#include "interfaces/IGameProfile.h"
#include <vector>
#include <memory>
#include <filesystem>

namespace GOW {

class ProfileManager {
public:
    static ProfileManager& Get() {
        static ProfileManager instance;
        return instance;
    }

    void RegisterProfile(std::shared_ptr<IGameProfile> profile);

    // Tenta achar um perfil compatível iterando sobre o Detect de todos
    std::shared_ptr<IGameProfile> DetectProfileForFile(const std::filesystem::path& path) const;

    // Busca por hint de nome (ex: "gowr", "gow2")
    std::shared_ptr<IGameProfile> FindProfileByHint(const std::string& hint) const;

    const std::vector<std::shared_ptr<IGameProfile>>& GetProfiles() const { return m_profiles; }

private:
    ProfileManager() = default;
    
    std::vector<std::shared_ptr<IGameProfile>> m_profiles;
};

} // namespace GOW
