#pragma once
#include "core/domain/Wad.h"
#include "core/types/GameVersion.h"
#include "core/types/TypeId.h"
#include "ui/IPanel.h"

class AssetDatabase;

class PakBrowser : public IPanel {
public:
    void Draw() override;
    std::string_view getName() const override { return "PAK Browser"; }

private:
    char m_filter[64] = "";
};
