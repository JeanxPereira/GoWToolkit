#pragma once
#include <Onyx/App/IPanel.h>
#include <Onyx/App/InfoTab.h>

class Inspector : public Onyx::App::IPanel {
public:
    void Draw() override;
    std::string_view getName() const override { return "Inspector"; }

private:
    Onyx::App::InfoTab m_info_tab;
};
