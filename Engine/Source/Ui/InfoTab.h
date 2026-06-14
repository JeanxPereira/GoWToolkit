#pragma once
#include "Core/AssetDatabase.h"

namespace Onyx::App {

class InfoTab {
public:
    void Draw(Onyx::Services::AssetDatabase& db, AssetEntry* e);
};

} // namespace Onyx::App
