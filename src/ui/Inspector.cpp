#include "ui/Inspector.h"
#include "UIHelpers.h"
#include "core/ToolkitApi.h"
#include "core/profiles/gowr/GowrProfileTag.h"
#include "fonts/SFSymbols.h"
#include "imgui.h"

#include "ui/viewers/DocumentWindow.h"
#include "ui/viewers/IDocumentContent.h"

void Inspector::Draw() {
    if (!visible) return;

    ImGui::Begin("Inspector", &visible);

    // Use the global selection state
    ParsedEntry* entry = GOW::Api::GetSelected();

    if (!entry) {
        ImGui::TextDisabled("No entry selected");
        ImGui::End();
        return;
    }

    // ── Header — always visible ─────────────────────────────────────────
    ImGui::PushID("InspectorHeader");

    const char* icon = IconForType(entry->typeId);
    if (auto* t = entry->profileTag.As<GOW::Gowr::GowrProfileTag>()) {
        if (t->role != GOW::Gowr::WadEntryRole::Unknown) {
            icon = IconForRole(t->role);
        }
    }

    ImGui::TextColored(ColorForType(entry->typeId),
        "%s  [%s]", icon, TypeName(entry->typeId));
    ImGui::TextWrapped("%s", entry->name.c_str());

    // Second line: WAD + size
    ImGui::TextDisabled("%s  |  %s", entry->wadName.c_str(),
                        FormatBytes(entry->size).c_str());

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
            char offsetStr[32];
            snprintf(offsetStr, sizeof(offsetStr), "0x%08X", entry->offset);
            if (ImGui::MenuItem(ICON_SF_ARROW_RIGHT "  Copy Offset")) {
                ImGui::SetClipboardText(offsetStr);
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
    ImGui::Separator();

    // ── Viewer Inspector section — if a document viewer is active ────────
    if (GOW::Api::Documents().HasActiveDocument()) {
        auto doc = GOW::Api::Documents().GetActiveDocument();
        if (doc) {
            doc->DrawInspector();
            ImGui::Separator();
        }
    }

    // ── Properties section — always shown (collapsible) ─────────────────
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        m_info_tab.Draw(GOW::Api::Database(), entry);
    }

    ImGui::End();
}