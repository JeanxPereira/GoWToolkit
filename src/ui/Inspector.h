#pragma once
#include "core/AssetDatabase.h"
#include "ui/IPanel.h"
#include "ui/InfoTab.h"

class Inspector : public IPanel {
public:
    void draw(AppContext& ctx) override;
    std::string_view getName() const override { return "Inspector"; }

private:
    InfoTab m_info_tab;
};
