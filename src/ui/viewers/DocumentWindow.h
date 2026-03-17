#pragma once
#include "IDocumentContent.h"
#include <vector>
#include <memory>

namespace GOW {

class DocumentWindow {
public:
    void AddTab(std::shared_ptr<IDocumentContent> tab);
    void Draw();
    void CloseAll();
    void CloseActiveTab();

private:
    std::vector<std::shared_ptr<IDocumentContent>> m_tabs;
    int m_activeTabIndex = -1; // Track the currently active tab
};

} // namespace GOW
