#pragma once
#include <Onyx/App/IPanel.h>
#include <Onyx/Domain/MediaKind.h>
#include <Onyx/Domain/Wad.h>
#include <Onyx/Modules/Selection.h>
#include <Onyx/Services/EventBus.h>

#include <filesystem>
#include <map>

namespace Onyx::Modules { class Workspace; }

// GoW-specific asset browser.
//
// Onyx v1.1 ships App::DocumentBrowser, a generic tree over the Workspace,
// and this panel is deliberately not replaced by it: DocumentBrowser draws
// names and TypeCatalog icons and posts SelectionChanged, while this one also
// carries the text filter, the MediaKind filter, the AssetVisibility gate,
// GOWR role-based icons and colours, and the open/extract/"view all textures"
// actions. Those are the reasons someone uses this app rather than a generic
// container viewer.
//
// What DID change is where the tree comes from. Onyx::Services::AssetDatabase
// is gone; documents live on the Workspace, selection is an event rather than
// a global pointer, and a NodePath (not an AssetEntry*) is what survives a
// reparse.
class WadBrowser : public Onyx::App::IPanel {
public:
    explicit WadBrowser(Onyx::Modules::Workspace& workspace);
    ~WadBrowser();

    void Draw() override;
    std::string_view getName() const override { return "WAD Browser"; }

private:
    // ViewerRegistry::Open and MapViewer still take a Domain::AssetContainer&,
    // which v1.1 keeps for exactly this reason, so each open document gets one
    // bridge built from its roots + file table (the same projection
    // AssetHarness::LoadContainer makes headlessly). Held in a map rather than
    // rebuilt per frame because viewers capture the reference: map nodes have
    // stable addresses, a vector's would move on rehash.
    Onyx::Domain::AssetContainer& BridgeFor(Onyx::Modules::Document& doc);

    Onyx::Modules::Workspace& m_workspace;
    std::map<Onyx::Modules::DocumentId, Onyx::Domain::AssetContainer> m_bridges;

    Onyx::Modules::DocumentId    m_selDoc = 0;
    Onyx::Modules::NodePath      m_selPath;
    Onyx::Services::Subscription m_openedSub;
    Onyx::Services::Subscription m_closedSub;

    char m_filter[128] = {};
    int  m_kindFilterIndex = 0; // 0 = All
};
