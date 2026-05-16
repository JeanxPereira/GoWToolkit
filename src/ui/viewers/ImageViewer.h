#pragma once
#include "IDocumentContent.h"
#include "core/parsers/shared/TextureData.h"
#include <imgui.h>
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
    bool m_showAlpha = false;

    // Pan + zoom state. *Target values are the immediate result of input;
    // unsuffixed values lerp toward them each frame for smooth motion.
    float  m_zoom       = 1.0f;
    float  m_zoomTarget = 1.0f;
    ImVec2 m_pan{0, 0};
    ImVec2 m_panTarget{0, 0};
    bool   m_panInitialized = false;

    void UploadToGPU();
};

} // namespace GOW
