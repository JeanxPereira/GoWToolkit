#include "ProfileManager.h"

namespace Onyx {

void ProfileManager::RegisterProfile(std::shared_ptr<IAssetProfile> profile) {
    if (profile) {
        m_profiles.push_back(std::move(profile));
    }
}

std::shared_ptr<IAssetProfile> ProfileManager::DetectProfileForFile(const std::filesystem::path& path) const {
    for (const auto& profile : m_profiles) {
        if (profile->Detect(path)) {
            return profile;
        }
    }
    return nullptr;
}

std::shared_ptr<IAssetProfile> ProfileManager::FindProfileByHint(const std::string& hint) const {
    std::string hintLower = hint;
    for (auto& c : hintLower) c = (char)tolower(c);

    // Short aliases recognized before substring search
    // Maps common CLI hints → substring that will match the full profile name
    const std::pair<const char*, const char*> aliases[] = {
        {"gow1",      "god of war i"},
        {"gow2",      "god of war ii"},
        {"gowr",      "ragnarok"},
        {"ragnarok",  "ragnarok"},
        {"ps2",       "ps2"},
        {"ps4",       "ps4"},
        {"ps5",       "ps5"},
    };
    for (auto [alias, expanded] : aliases) {
        if (hintLower == alias) { hintLower = expanded; break; }
    }

    for (const auto& profile : m_profiles) {
        std::string nameLower = profile->GetName();
        for (auto& c : nameLower) c = (char)tolower(c);
        if (nameLower.find(hintLower) != std::string::npos)
            return profile;
    }
    return nullptr;
}

} // namespace Onyx
