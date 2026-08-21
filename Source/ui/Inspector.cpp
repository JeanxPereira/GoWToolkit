#include "ui/Inspector.h"
#include <Onyx/App/UIHelpers.h>
#include "ui/RoleVisuals.h"   // GOWR role → color/icon (ColorForRole, IconForRole)
#include <Onyx/Api/ToolkitApi.h>
#include "core/profiles/gowr/GowrTaxonomy.h"
#include "core/WadTypes.h"
#include <Onyx/Fonts/SFSymbols.h>
#include "imgui.h"

#include <Onyx/Modules/Workspace.h>
#include <Onyx/Viewers/DocumentWindow.h>
#include <Onyx/Viewers/IDocumentContent.h>

#include <cstdio>

// Mirrors Onyx's own InfoTab: capture the DocumentId + NodePath the event
// carries, never an AssetEntry*, and re-resolve the path each frame. A
// document can reparse to a smaller tree or close entirely between the click
// and the draw, and a stored pointer would dangle where a path merely stops
// resolving.
Inspector::Inspector(Onyx::Modules::Workspace& workspace)
    : m_workspace(workspace),
      m_info_tab(workspace),
      m_selectionSub(workspace.Events().On<Onyx::Modules::SelectionChanged>(
          [this](const Onyx::Modules::SelectionChanged& ev) {
              m_selDoc       = ev.doc;
              m_selPath      = ev.path;
              m_hasSelection = true;
          })) {}

void Inspector::Draw() {
    if (!visible) return;

    ImGui::Begin("Inspector", &visible);

    const Onyx::Domain::AssetEntry* entry = nullptr;
    if (m_hasSelection) {
        Onyx::Modules::Document* doc = m_workspace.Get(m_selDoc);
        // Workspace.h's thread rule: only `ready` is safe to poll on a
        // document that may still be parsing on a worker thread. A closed
        // document (Get() returns null) is the same "nothing to show" case.
        if (doc && doc->ready.load())
            entry = Onyx::Modules::Resolve(*doc, m_selPath);
    }

    if (!entry) {
        ImGui::TextDisabled(m_hasSelection ? "Selection stale" : "No entry selected");
        ImGui::End();
        return;
    }

    // ── Header — always visible ─────────────────────────────────────────
    ImGui::PushID("InspectorHeader");

    const char* icon = IconForType(entry->typeId);
    // Onyx v1.1 removed AssetEntry::profileTag; role is reclassified on
    // demand from the entry's own name + size via Gowr::Classify().
    Onyx::Gowr::WadEntryRole role = Onyx::Gowr::Classify(*entry).role;
    if (role != Onyx::Gowr::WadEntryRole::Unknown) {
        icon = IconForRole(role);
    }

    ImGui::TextColored(ColorForType(entry->typeId),
        "%s  [%s]", icon, TypeName(entry->typeId));
    ImGui::TextWrapped("%s", entry->name.c_str());

    // Second line: WAD + size
    ImGui::TextDisabled("%s  |  %s", entry->wadName.c_str(),
                        FormatBytes(entry->source.size).c_str());

    // ── Context menu on header ──────────────────────────────────────────
    if (ImGui::BeginPopupContextItem("InspectorHeaderCtx")) {
        if (ImGui::MenuItem(ICON_SF_DOCUMENT_ON_DOCUMENT "  Copy Name")) {
            ImGui::SetClipboardText(entry->name.c_str());
        }
        if (entry->hash != 0) {
            if (ImGui::MenuItem(ICON_SF_NUMBER "  Copy Hash")) {
                ImGui::SetClipboardText(HashHex(entry->hash).c_str());
            }
        }
        {
            // ByteRange::offset is 64-bit in v1.1; the old %08X on a
            // uint32_t would truncate an ISO-mounted entry's real offset.
            char offsetStr[32];
            snprintf(offsetStr, sizeof(offsetStr), "0x%08llX",
                     static_cast<unsigned long long>(entry->source.offset));
            if (ImGui::MenuItem(ICON_SF_ARROW_RIGHT "  Copy Offset")) {
                ImGui::SetClipboardText(offsetStr);
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
    ImGui::Separator();

    // ── Viewer Inspector section — if a document viewer is active ────────
    if (Onyx::Api::Documents().HasActiveDocument()) {
        auto doc = Onyx::Api::Documents().GetActiveDocument();
        if (doc) {
            doc->DrawInspector();
            ImGui::Separator();
        }
    }

    // ── Properties section — always shown (collapsible) ─────────────────
    // InfoTab carries its own subscription to the same event and resolves
    // the path itself, so it needs no arguments -- the AssetDatabase&/
    // AssetEntry* overload died with AssetDatabase.
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        m_info_tab.Draw();
    }

    ImGui::End();
}
