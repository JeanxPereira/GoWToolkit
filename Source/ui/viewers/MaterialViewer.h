#pragma once

#include <Onyx/Viewers/IDocumentContent.h>
#include "core/parsers/gow2/MaterialParser.h"
#include <Onyx/Parsers/TextureData.h>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <imgui.h>

namespace Onyx::App { class TexturePool; }

namespace Onyx {

class MaterialViewer : public Onyx::Viewers::IDocumentContent {
public:
    // The TextureLookupFn parameter is gone. Its only caller
    // (MaterialHandler.cpp) already passed a lambda returning 0 with the
    // comment "textures are now passed directly", so the fallback path it fed
    // had been dead before this port; under Vulkan it could not have worked at
    // all, since a raw GL name is not an ImTextureID.
    MaterialViewer(const std::string& name,
                   std::unique_ptr<GOW2MaterialParser::MaterialData> matData,
                   std::vector<std::unique_ptr<Parsers::TextureData>> textures = {});
    
    ~MaterialViewer() override;

    std::string GetName() const override;
    void Draw() override;

private:
    void UploadTextures();

    std::string m_name;
    std::unique_ptr<GOW2MaterialParser::MaterialData> m_matData;

    // Owned texture data and their pooled Vulkan handles. This viewer used to
    // keep its own GL texture names and hand them to ImGui::Image directly;
    // v1.1 has no GL context at all, and TexturePool is the supported way for
    // a viewer to get an ImTextureID (the same route Onyx's own ImageViewer
    // takes). The pool is owned per viewer, per TexturePool.h's ownership
    // model, and destroying it releases every texture it made.
    std::vector<std::unique_ptr<Parsers::TextureData>> m_textures;
    std::unique_ptr<Onyx::App::TexturePool> m_texPool;
    std::vector<ImTextureID> m_texIds;        // parallel to m_textures
    int m_selectedLayer = 0;                  // which layer is selected for preview
};

} // namespace Onyx
