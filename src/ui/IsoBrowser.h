#pragma once
#include "core/AssetDatabase.h"
#include "ui/IPanel.h"

class IsoBrowser : public IPanel {
public:
    void draw(AppContext& ctx) override;
    std::string_view getName() const override { return "ISO Browser"; }

private:
    char m_filter[64] = "";
};
