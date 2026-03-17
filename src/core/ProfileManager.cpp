#include "ProfileManager.h"

namespace GOW {

void ProfileManager::RegisterProfile(std::shared_ptr<IGameProfile> profile) {
    if (profile) {
        m_profiles.push_back(std::move(profile));
    }
}

std::shared_ptr<IGameProfile> ProfileManager::DetectProfileForFile(const std::filesystem::path& path) const {
    for (const auto& profile : m_profiles) {
        if (profile->Detect(path)) {
            return profile;
        }
    }
    return nullptr;
}

std::shared_ptr<IGameProfile> ProfileManager::FindProfileByHint(const std::string& hint) const {
    for (const auto& profile : m_profiles) {
        auto name = profile->GetName();
        // case-insensitive substring match
        std::string nameLower = name;
        std::string hintLower = hint;
        for (auto& c : nameLower) c = (char)tolower(c);
        for (auto& c : hintLower) c = (char)tolower(c);
        if (nameLower.find(hintLower) != std::string::npos)
            return profile;
    }
    return nullptr;
}

} // namespace GOW
