#pragma once
#include <vector>
#include <memory>
#include <string_view>
#include <Onyx/App/IPanel.h>

namespace Onyx::App {

class PanelRegistry {
public:
    void add(std::unique_ptr<IPanel> panel) {
        m_panels.push_back(std::move(panel));
    }

    void DrawAll() {
        for (auto& p : m_panels) {
            if (p->visible)
                p->Draw();
        }
    }

    IPanel* find(std::string_view name) {
        for (auto& p : m_panels) {
            if (p->getName() == name)
                return p.get();
        }
        return nullptr;
    }

    // Iterator support for View menu toggles etc.
    auto begin() { return m_panels.begin(); }
    auto end()   { return m_panels.end(); }
    auto begin() const { return m_panels.begin(); }
    auto end()   const { return m_panels.end(); }

private:
    std::vector<std::unique_ptr<IPanel>> m_panels;
};

} // namespace Onyx::App
