#pragma once
#include "IDocumentContent.h"
#include <vector>
#include <memory>

namespace Onyx::Viewers {

class DocumentWindow {
public:
    DocumentWindow();
    ~DocumentWindow();

    void AddTab(std::shared_ptr<IDocumentContent> tab);
    void Draw();
    void CloseAll();
    void CloseActiveTab();
    
    std::shared_ptr<IDocumentContent> GetActiveDocument() const;
    bool HasActiveDocument() const;

private:
    std::vector<std::shared_ptr<IDocumentContent>> m_tabs;
    int m_activeTabIndex = -1; // Track the currently active tab

    // Tabs queued for destruction. We can't release a viewer's shared_ptr in
    // the middle of the ImGui frame: ImGui::Image() may already have queued
    // the viewer's FBO texture ID into the current draw list. If the dtor
    // runs now (Viewport3D ~ glDeleteTextures), the bound GL texture is freed
    // before ImGui::Render submits it â€” the GPU then samples whatever is
    // still bound (typically the font atlas), producing a one-frame flash of
    // stretched glyphs. We carry the strong references for one extra frame
    // so the draw call completes before the resources go away.
    std::vector<std::shared_ptr<IDocumentContent>> m_pendingDelete;
};

} // namespace Onyx::Viewers
