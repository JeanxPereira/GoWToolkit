#pragma once
#include "core/AssetDatabase.h"
#include "ui/IPanel.h"
#include <filesystem>
#include "core/domain/MediaKind.h"

class WadBrowser : public IPanel {
public:
    void draw(AppContext& ctx) override;
    std::string_view getName() const override { return "WAD Browser"; }

private:
    char m_filter[128] = {};
    int  m_kindFilterIndex = 0; // 0 = All
};
