#pragma once
#include <vector>
#include <memory>
#include <string_view>
#include "ui/IPanel.h"

struct AppContext;

class PanelRegistry {
public:
    void add(std::unique_ptr<IPanel> panel) {
        m_panels.push_back(std::move(panel));
    }

    void drawAll(AppContext& ctx) {
        for (auto& p : m_panels) {
            if (p->visible)
                p->draw(ctx);
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
