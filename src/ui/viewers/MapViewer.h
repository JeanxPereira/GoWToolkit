#pragma once

#include "ui/viewers/IDocumentContent.h"
#include "ui/viewers/Viewport3D.h"
#include "core/WadTypes.h"
#include <map>
#include <memory>
#include <string>

namespace GOW {

class MapViewer : public IDocumentContent {
public:
    MapViewer(const std::string& wadName, OpenWad& wad);
    ~MapViewer() override = default;

    std::string GetName() const override { return "Map: " + m_wadName; }
    void Draw() override;
    void DrawInspector(AppContext& ctx) override;
    
    // Aggregates all CXT_* entries in the WAD into a single combined SceneData
    void LoadMap();

private:
    std::string m_wadName;
    OpenWad& m_wad;
    
    std::unique_ptr<Viewport3D> m_viewport;
    bool m_isLoading = false;
    bool m_isLoaded = false;
    
    float m_loadProgress = 0.0f;
    std::string m_loadStatus;
};

} // namespace GOW
