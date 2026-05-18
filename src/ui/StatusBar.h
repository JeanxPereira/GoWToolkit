#pragma once
#include "ui/IPanel.h"

class StatusBar : public IPanel {
public:
    int selectedLog = -1;
    void draw(AppContext& ctx) override;
    std::string_view getName() const override { return "Log"; }
    void SetMessage(const char* msg);
};
