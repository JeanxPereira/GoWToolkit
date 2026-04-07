#include "DocumentWindow.h"
#include "imgui.h"

namespace GOW {

void DocumentWindow::AddTab(std::shared_ptr<IDocumentContent> tab) {
    if (tab) {
        m_tabs.push_back(tab);
    }
}

void DocumentWindow::CloseAll() {
    m_tabs.clear();
    m_activeTabIndex = -1;
}

void DocumentWindow::CloseActiveTab() {
    if (m_activeTabIndex >= 0 && m_activeTabIndex < (int)m_tabs.size()) {
        m_tabs.erase(m_tabs.begin() + m_activeTabIndex);
        m_activeTabIndex = -1; // Reset until next draw loop updates it
    }
}

bool DocumentWindow::HasActiveDocument() const {
    return m_activeTabIndex >= 0 && m_activeTabIndex < (int)m_tabs.size();
}

std::shared_ptr<IDocumentContent> DocumentWindow::GetActiveDocument() const {
    if (HasActiveDocument()) return m_tabs[m_activeTabIndex];
    return nullptr;
}

void DocumentWindow::Draw() {
    if (m_tabs.empty()) {
        ImGui::Begin("Viewer");
        ImGui::TextDisabled("No documents open.");
        ImGui::End();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewer");
    ImGui::PopStyleVar();

    if (ImGui::BeginTabBar("DocumentTabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
        for (size_t i = 0; i < m_tabs.size(); ) {
            auto& tab = m_tabs[i];
            bool open = tab->IsOpen();
            
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            std::string tabTitle = tab->GetName() + "###" + std::to_string(reinterpret_cast<uintptr_t>(tab.get()));
            
            if (ImGui::BeginTabItem(tabTitle.c_str(), &open, flags)) {
                m_activeTabIndex = (int)i; // Track active tab
                tab->Draw();
                ImGui::EndTabItem();
            }
            
            if (!open) {
                m_tabs.erase(m_tabs.begin() + i);
                if (m_activeTabIndex == (int)i) m_activeTabIndex = -1;
            } else {
                tab->SetOpen(open);
                ++i;
            }
        }
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

} // namespace GOW
