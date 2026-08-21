#pragma once
#include <Onyx/App/IPanel.h>
#include <Onyx/App/InfoTab.h>
#include <Onyx/Modules/Selection.h>
#include <Onyx/Services/EventBus.h>

namespace Onyx::Modules { class Workspace; }

// GoW-specific Inspector.
//
// Onyx v1.1 ships its own App::InspectorPanel, which hosts an InfoTab and
// nothing else. This one survives instead of being replaced by it because
// of its header: the icon and colour it shows come from the GOWR role
// taxonomy (Gowr::Classify -- the reverse-engineered naming rules Phase 2
// kept alive after AssetEntry::profileTag was removed), which the generic
// panel has no way to know about.
//
// Selection reaches it the v1.1 way. Onyx::Api::GetSelected() and
// Api::Database() are gone along with AssetDatabase; selection is now a
// SelectionChanged{DocumentId, NodePath} event on the Workspace's bus, and
// a holder re-resolves the path every frame rather than caching an
// AssetEntry* that a reparse would dangle.
class Inspector : public Onyx::App::IPanel {
public:
    explicit Inspector(Onyx::Modules::Workspace& workspace);

    void Draw() override;
    std::string_view getName() const override { return "Inspector"; }

private:
    Onyx::Modules::Workspace&    m_workspace;
    Onyx::App::InfoTab           m_info_tab;
    Onyx::Modules::DocumentId    m_selDoc = 0;
    Onyx::Modules::NodePath      m_selPath;
    bool                         m_hasSelection = false;
    Onyx::Services::Subscription m_selectionSub;
};
