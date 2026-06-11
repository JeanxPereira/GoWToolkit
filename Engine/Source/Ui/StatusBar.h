#pragma once
#include "Ui/IPanel.h"

class StatusBar : public IPanel {
public:
    StatusBar();
    ~StatusBar();
    int selectedLog = -1;
    void Draw() override;
    std::string_view getName() const override { return "Log"; }
    void SetMessage(const char* msg);
};
