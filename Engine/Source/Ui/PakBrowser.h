#pragma once
#include "Core/Domain/Wad.h"
#include "Core/Types/GameVersion.h"
#include "Core/Types/TypeId.h"
#include "Ui/IPanel.h"

class AssetDatabase;

namespace Onyx::App {

class PakBrowser : public IPanel {
public:
    void Draw() override;
    std::string_view getName() const override { return "PAK Browser"; }

private:
    char m_filter[64] = "";
};

} // namespace Onyx::App
