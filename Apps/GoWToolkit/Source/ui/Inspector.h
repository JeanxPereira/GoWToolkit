#pragma once
#include "Ui/IPanel.h"
#include "Ui/InfoTab.h"

class Inspector : public IPanel {
public:
    void Draw() override;
    std::string_view getName() const override { return "Inspector"; }

private:
    InfoTab m_info_tab;
};
