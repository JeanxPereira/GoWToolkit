#include "ui/Inspector.h"
#include "UIHelpers.h"
#include "ui/RoleVisuals.h"   // GOWR role â†’ color/icon (ColorForRole, IconForRole)
#include "Core/ToolkitApi.h"
#include "core/profiles/gowr/GowrProfileTag.h"
#include "Fonts/SFSymbols.h"
#include "imgui.h"

#include "Ui/Viewers/DocumentWindow.h"
#include "Ui/Viewers/IDocumentContent.h"

void Inspector::Draw() {
    if (!visible) return;

    ImGui::Begin("Inspector", &visible);

    // Use the global selection state
    AssetEntry* entry = Onyx::Api::GetSelected();

    if (!entry) {
        ImGui::TextDisabled("No entry selected");
        ImGui::End();
        return;
    }

    // â”€â”€ Header â€” always visible â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImGui::PushID("InspectorHeader");

    const char* icon = IconForType(entry->typeId);
    if (auto* t = entry->profileTag.As<Onyx::Gowr::GowrProfileTag>()) {
        if (t->role != Onyx::Gowr::WadEntryRole::Unknown) {
            icon = IconForRole(t->role);
        }
    }

    ImGui::TextColored(ColorForType(entry->typeId),
        "%s  [%s]", icon, TypeName(entry->typeId));
    ImGui::TextWrapped("%s", entry->name.c_str());

    // Second line: WAD + size
    ImGui::TextDisabled("%s  |  %s", entry->wadName.c_str(),
                        FormatBytes(entry->size).c_str());

    // â”€â”€ Context menu on header â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

    // â”€â”€ Viewer Inspector section â€” if a document viewer is active â”€â”€â”€â”€â”€â”€â”€â”€
    if (Onyx::Api::Documents().HasActiveDocument()) {
        auto doc = Onyx::Api::Documents().GetActiveDocument();
        if (doc) {
            doc->DrawInspector();
            ImGui::Separator();
        }
    }

    // â”€â”€ Properties section â€” always shown (collapsible) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        m_info_tab.Draw(Onyx::Api::Database(), entry);
    }

    ImGui::End();
}
