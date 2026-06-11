#pragma once
#include "Core/AssetDatabase.h"
#include "Ui/IPanel.h"
#include <filesystem>
#include "Core/Domain/MediaKind.h"

class WadBrowser : public IPanel {
public:
    WadBrowser();
    ~WadBrowser();
    void Draw() override;
    std::string_view getName() const override { return "WAD Browser"; }

private:
    char m_filter[128] = {};
    int  m_kindFilterIndex = 0; // 0 = All
};
