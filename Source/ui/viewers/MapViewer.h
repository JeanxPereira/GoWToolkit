#pragma once

#include <Onyx/Viewers/IDocumentContent.h>
#include <Onyx/Viewers/Viewport3D.h>
#include "core/WadTypes.h"
#include <map>
#include <memory>
#include <string>

namespace Onyx {

class MapViewer : public Onyx::Viewers::IDocumentContent {
public:
    MapViewer(const std::string& wadName, AssetContainer& wad);
    ~MapViewer() override = default;

    std::string GetName() const override { return "Map: " + m_wadName; }
    void Draw() override;
    void DrawInspector() override;
    Viewers::Viewport3D* GetEmbeddedViewport() override { return m_viewport.get(); }

    // Aggregates all CXT_* entries in the WAD into a single combined SceneData
    void LoadMap();

private:
    std::string m_wadName;
    AssetContainer& m_wad;

    std::unique_ptr<Viewers::Viewport3D> m_viewport;
    bool m_isLoading = false;
    bool m_isLoaded = false;
    
    float m_loadProgress = 0.0f;
    std::string m_loadStatus;
};

} // namespace Onyx
