#pragma once
#include <Onyx/Services/AssetDatabase.h>
#include <Onyx/App/IPanel.h>
#include <filesystem>
#include <Onyx/Domain/MediaKind.h>

class WadBrowser : public Onyx::App::IPanel {
public:
    WadBrowser();
    ~WadBrowser();
    void Draw() override;
    std::string_view getName() const override { return "WAD Browser"; }

private:
    char m_filter[128] = {};
    int  m_kindFilterIndex = 0; // 0 = All
};
