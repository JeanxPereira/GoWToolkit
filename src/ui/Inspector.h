#pragma once
#include "core/AssetDatabase.h"
#include "ui/IPanel.h"
#include "ui/InfoTab.h"

class Inspector : public IPanel {
public:
    Inspector();
    ~Inspector();

    void draw(AppContext& ctx) override;
    std::string_view getName() const override { return "Inspector"; }

private:
    InfoTab m_info_tab;
    ParsedEntry* m_selectedEntry = nullptr;
    OpenWad* m_selectedWad = nullptr;
};
