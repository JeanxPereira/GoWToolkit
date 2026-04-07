#pragma once
#include "IDocumentContent.h"
#include "core/parsers/shared/TextureData.h"
#include <string>
#include <memory>

namespace GOW {

class ImageViewer : public IDocumentContent {
public:
    ImageViewer(const std::string& name, std::unique_ptr<TextureData> texture);
    ~ImageViewer() override;

    std::string GetName() const override;
    void Draw() override;

private:
    std::string m_name;
    std::unique_ptr<TextureData> m_texture;
    unsigned int m_glTexture = 0;
    float m_zoom = 1.0f;
    bool m_showAlpha = false;

    void UploadToGPU();
};

} // namespace GOW
