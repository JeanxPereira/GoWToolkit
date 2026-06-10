#include "DocumentWindow.h"
#include "imgui.h"
#include "core/Events.h"
#include "ui/Widgets.h"

namespace Onyx {

DocumentWindow::DocumentWindow() {
    // Subscribe to EventAssetSelected — when an asset is selected in any browser,
    // DocumentWindow tracks it for potential future auto-preview behavior.
    // Currently this enables the Inspector to query active document state and
    // ensures DocumentWindow is wired into the event pipeline (M3.T2).
    EventAssetSelected::subscribe(this, [this](AssetEntry* /*entry*/, AssetContainer* /*wad*/) {
        // Track selection for future preview tab policy.
        // Explicit viewer opens (double-click, context menu) go through AddTab().
        // Auto-preview on single-click deferred to UX decision — see D0011.
    });

    EventWadClosed::subscribe(this, [this](size_t /*wadIdx*/) {
        // For now, close all tabs as a safe default when any WAD is closed.
        // In the future (M4), viewers should hold metadata about their source WAD to allow selective closing.
        CloseAll();
    });

    EventAllClosed::subscribe(this, [this]() {
        CloseAll();
    });
}

DocumentWindow::~DocumentWindow() {
    EventAssetSelected::unsubscribe(this);
    EventWadClosed::unsubscribe(this);
    EventAllClosed::unsubscribe(this);
}

void DocumentWindow::AddTab(std::shared_ptr<IDocumentContent> tab) {
    if (tab) {
        m_tabs.push_back(tab);
        EventDocumentOpened::post(tab.get());
    }
}

void DocumentWindow::CloseAll() {
    // Defer destruction to the next Draw() so GL resources (FBO textures
    // referenced by ImGui::Image in the current frame) live until after
    // ImGui::Render submits them.
    for (auto& t : m_tabs) m_pendingDelete.push_back(std::move(t));
    m_tabs.clear();
    m_activeTabIndex = -1;
}

void DocumentWindow::CloseActiveTab() {
    if (m_activeTabIndex >= 0 && m_activeTabIndex < (int)m_tabs.size()) {
        m_pendingDelete.push_back(std::move(m_tabs[m_activeTabIndex]));
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
    // Release viewers queued last frame — the ImGui draw list that referenced
    // their textures has already been rendered and presented.
    if (!m_pendingDelete.empty()) m_pendingDelete.clear();

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
            
            if (Onyx::UI::Widgets::BeginTabItem(tabTitle.c_str(), &open, flags)) {
                m_activeTabIndex = (int)i; // Track active tab
                tab->Draw();
                ImGui::EndTabItem();
            }
            
            if (!open) {
                // Hand the strong reference to m_pendingDelete so the viewer
                // (and any GL textures it owns) outlive this frame's draw
                // submission — see m_pendingDelete comment in the header.
                m_pendingDelete.push_back(std::move(m_tabs[i]));
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

} // namespace Onyx
