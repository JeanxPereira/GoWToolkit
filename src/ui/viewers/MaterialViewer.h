#pragma once

#include "IDocumentContent.h"
#include "core/parsers/gow2/MaterialParser.h"
#include "core/parsers/shared/TextureData.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>

using GLuint = unsigned int;

namespace GOW {

class MaterialViewer : public IDocumentContent {
public:
    using TextureLookupFn = std::function<unsigned int(const std::string&)>;

    MaterialViewer(const std::string& name, 
                   std::unique_ptr<GOW2MaterialParser::MaterialData> matData,
                   TextureLookupFn texLookup,
                   std::vector<std::unique_ptr<TextureData>> textures = {});
    
    ~MaterialViewer() override;

    std::string GetName() const override;
    void Draw() override;

private:
    void UploadTextures();

    std::string m_name;
    std::unique_ptr<GOW2MaterialParser::MaterialData> m_matData;
    TextureLookupFn m_texLookup;

    // Owned texture data & GL handles
    std::vector<std::unique_ptr<TextureData>> m_textures;
    std::vector<GLuint> m_glTextures;         // parallel to m_textures
    int m_selectedLayer = 0;                  // which layer is selected for preview
};

} // namespace GOW