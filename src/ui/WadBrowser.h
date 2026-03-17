#pragma once
#include "core/AssetDatabase.h"
#include "ui/IPanel.h"
#include <filesystem>

class WadBrowser : public IPanel {
public:
    void draw(AppContext& ctx) override;
    std::string_view getName() const override { return "WAD Browser"; }

private:
    char m_filter[128] = {};
};
