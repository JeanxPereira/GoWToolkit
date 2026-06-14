#pragma once
#include "Core/AssetDatabase.h"
#include "Ui/IPanel.h"

namespace Onyx::App {

class IsoBrowser : public IPanel {
public:
    void Draw() override;
    std::string_view getName() const override { return "ISO Browser"; }

private:
    char m_filter[64] = "";
};

} // namespace Onyx::App
