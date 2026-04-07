#pragma once
#include "core/WadTypes.h"
#include "ui/IPanel.h"

class AssetDatabase;

class PakBrowser : public IPanel {
public:
    void draw(AppContext& ctx) override;
    std::string_view getName() const override { return "PAK Browser"; }

private:
    char m_filter[64] = "";
};
