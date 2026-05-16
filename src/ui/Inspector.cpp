#include "ui/Inspector.h"
#include "ui/AppContext.h"
#include "UIHelpers.h"
#include "imgui.h"
#include "ui/viewers/DocumentWindow.h"
#include "ui/viewers/IDocumentContent.h"
#include "core/Events.h"

Inspector::Inspector() {
    EventAssetSelected::subscribe(this, [this](ParsedEntry* entry, OpenWad* wad) {
        m_selectedEntry = entry;
        m_selectedWad = wad;
    });

    EventWadClosed::subscribe(this, [this](size_t /* wadIdx */) {
        m_selectedEntry = nullptr;
        m_selectedWad = nullptr;
    });

    EventAllClosed::subscribe(this, [this]() {
        m_selectedEntry = nullptr;
        m_selectedWad = nullptr;
    });
}

Inspector::~Inspector() {
    EventAssetSelected::unsubscribe(this);
    EventWadClosed::unsubscribe(this);
    EventAllClosed::unsubscribe(this);
}

void Inspector::draw(AppContext& ctx) {
    if (!visible) return;

    ImGui::Begin("Inspector", &visible);

    if (ctx.documentWindow.HasActiveDocument()) {
        auto doc = ctx.documentWindow.GetActiveDocument();
        doc->DrawInspector(ctx);
        ImGui::End();
        return;
    }

    ParsedEntry* entry = m_selectedEntry;

    if (!entry) {
        ImGui::TextDisabled("No entry selected");
        ImGui::End();
        return;
    }

    // Header with type + name
    ImGui::TextColored(ColorForType(GOW::GameVersion::GOW2, entry->typeId, entry->schemaType),
        "[%s]", TypeName(GOW::GameVersion::GOW2, entry->typeId, entry->schemaType));
    ImGui::SameLine();
    ImGui::TextUnformatted(entry->name.c_str());
    ImGui::Separator();

    if (ImGui::BeginTabBar("##inspector_tabs")) {

        if (ImGui::BeginTabItem("Info")) {
            m_info_tab.Draw(ctx.db, entry);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Anims")) {
            ImGui::TextDisabled("(parser coming soon)");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Hex")) {
            ImGui::TextDisabled("(hex viewer coming soon)");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    // Export bar fixed at footer
    ImGui::SetCursorPosY(
        ImGui::GetWindowHeight() - 40);
    ImGui::Separator();

    if (ImGui::Button("Export glTF", ImVec2(110, 0)))
        {}
    ImGui::SameLine();
    if (ImGui::Button("Export DDS", ImVec2(110, 0)))
        {}
    ImGui::SameLine();
    if (ImGui::Button("Copy Hash", ImVec2(90, 0)))
        ImGui::SetClipboardText(HashHex(entry->hash).c_str());

    ImGui::End();
}