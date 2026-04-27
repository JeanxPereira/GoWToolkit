#include "ui/WadBrowser.h"
#include "UIHelpers.h"
#include "core/AssetDatabase.h"
#include "core/Logger.h"
#include "core/WadTypes.h"
#include "fonts/SFSymbols.h"
#include "imgui.h"
#include "ui/AppContext.h"
#include "ui/ViewerRegistry.h"
#include "ui/viewers/DocumentWindow.h"
#include "ui/viewers/MapViewer.h"
#include <functional>
#include <string>
#include <fstream>


void WadBrowser::draw(AppContext &ctx) {
  if (!visible)
    return;
  ImGui::Begin("WAD Browser", &visible);

  auto &db = ctx.db;

  if (db.wads.empty()) {
    ImGui::TextDisabled("No WAD loaded");
    ImGui::End();
    return;
  }

  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint("##filter", "Filter entries...", m_filter,
                           sizeof(m_filter));
  std::string filterLower(m_filter);
  for (auto &c : filterLower)
    c = (char)tolower(c);
  bool hasFilter = !filterLower.empty();

  ImGui::Separator();

  for (size_t wadIdx = 0; wadIdx < db.wads.size(); ++wadIdx) {
    auto &wad = db.wads[wadIdx];
    ImGui::PushID((int)wadIdx);
    // Botões à direita: Map, Close
    float btnMapParams = ImGui::CalcTextSize(ICON_SF_MAP_FILL).x;
    float btnCloseParams = ImGui::CalcTextSize(ICON_SF_XMARK).x;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float buttonsWidth = btnMapParams + btnCloseParams +
                         (ImGui::GetStyle().FramePadding.x * 4.0f) + spacing;
    float windowWidth = ImGui::GetContentRegionAvail().x;

    bool wadOpen =
        ImGui::TreeNodeEx(wad.filename.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    ImGui::SameLine(windowWidth - buttonsWidth);
    ImGui::SetNextItemAllowOverlap();
    if (ImGui::SmallButton(ICON_SF_MAP_FILL)) {
      auto viewer = std::make_shared<GOW::MapViewer>(wad.filename, wad);
      ctx.documentWindow.AddTab(viewer);
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Load Entire Map/Level");

    ImGui::SameLine();
    ImGui::SetNextItemAllowOverlap();
    if (ImGui::SmallButton(ICON_SF_XMARK)) {
      if (wadOpen)
        ImGui::TreePop();
      ImGui::PopID();
      db.CloseWad(wadIdx);
      break;
    }

    if (wadOpen) {
      int entryIdx = 0;

      // Recursive lambda for rendering tree
      std::function<void(ParsedEntry &, int &)> renderEntryTree;
      renderEntryTree = [&](ParsedEntry &entry, int &idx) {
        // Filter: check name match
        if (hasFilter) {
          std::string nameLower = entry.name;
          for (auto &c : nameLower)
            c = (char)tolower(c);
          std::string typeLower = entry.schemaType;
          for (auto &c : typeLower)
            c = (char)tolower(c);

          bool matchesFilter =
              (nameLower.find(filterLower) != std::string::npos ||
               typeLower.find(filterLower) != std::string::npos);
          if (!matchesFilter && entry.children.empty())
            return;
        }

        ImGui::PushID(idx);
        bool has_children = !entry.children.empty();

        // ── Flags ────────────────────────────────────────────
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!has_children) {
          flags |=
              ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        } else {
          flags |= ImGuiTreeNodeFlags_OpenOnArrow |
                   ImGuiTreeNodeFlags_OpenOnDoubleClick;
        }
        if (hasFilter)
          flags |= ImGuiTreeNodeFlags_DefaultOpen;
        if (ctx.selected == &entry)
          flags |= ImGuiTreeNodeFlags_Selected;

        // ── Icon + color (prefer role-based for GOWR entries) ────
        const char *icon;
        ImVec4 color;
        if (entry.role != WadEntryRole::Unknown) {
          icon = IconForRole(entry.role);
          color = ColorForRole(entry.role);
        } else {
          icon = IconForType(GOW::GameVersion::GOW2, entry.typeId,
                             entry.schemaType);
          color = ColorForType(GOW::GameVersion::GOW2, entry.typeId,
                               entry.schemaType);
        }

        // Use displayName if set, otherwise fall back to name
        const std::string &label_name =
            entry.displayName.empty() ? entry.name : entry.displayName;

        // ── TreeNode with formatted label ────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        char label[256];
        snprintf(label, sizeof(label), "%s  %s", icon, label_name.c_str());
        bool node_open = ImGui::TreeNodeEx(label, flags);
        ImGui::PopStyleColor();

        // ── Selection (single click) ─────────────────────────
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
          ctx.selected = &entry;
          db.EnsureNodeData(ctx.selected, wad);
        }

        // ── Double-click action ────────────────────────────────
        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          if (wad.fileSource) {
            auto viewer = ctx.viewerRegistry.Open(entry, wad);
            if (viewer)
              ctx.documentWindow.AddTab(viewer);
          }
        }

        // ── Tooltip on hover ─────────────────────────────────
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
          ImGui::BeginTooltip();
          ImGui::Text("Type: %s", entry.schemaType.c_str());
          ImGui::Text("Offset: 0x%08X", entry.offset);
          ImGui::Text("Size: %s", FormatBytes(entry.size).c_str());
          ImGui::EndTooltip();
        }

        // ── Right-click Context Menu ─────────────────────────
        if (ImGui::BeginPopupContextItem()) {
          ImGui::TextDisabled("%s", entry.name.c_str());
          ImGui::TextDisabled("%s  |  %s", entry.schemaType.c_str(),
                              FormatBytes(entry.size).c_str());
          ImGui::Separator();

          if (wad.fileSource &&
              ctx.viewerRegistry.CanHandle(GOW::GameVersion::GOW2, entry.typeId,
                                           entry.schemaType)) {
            auto title = std::string(ICON_SF_FOLDER_FILL) + "  Open";
            if (ImGui::MenuItem(title.c_str())) {
              auto viewer = ctx.viewerRegistry.Open(entry, wad);
              if (viewer)
                ctx.documentWindow.AddTab(viewer);
            }
          }

          // Type-specific extras: "View All Textures" for MDL with TXR children
          if (entry.schemaType == "GOW2_MDL" && has_children &&
              wad.fileSource) {
            int txrCount = 0;
            for (const auto &c : entry.children) {
              if (c.schemaType == "GOW2_TXR")
                txrCount++;
            }
            if (txrCount > 0) {
              char menuLabel[64];
              snprintf(menuLabel, sizeof(menuLabel),
                       ICON_SF_PHOTO " View All Textures (%d)", txrCount);
              if (ImGui::MenuItem(menuLabel)) {
                for (const auto &c : entry.children) {
                  if (c.schemaType == "GOW2_TXR") {
                    auto viewer = ctx.viewerRegistry.Open(c, wad);
                    if (viewer)
                      ctx.documentWindow.AddTab(viewer);
                  }
                }
              }
            }
          }

          if (ImGui::MenuItem(ICON_SF_DOCUMENT_ON_DOCUMENT " Copy Name")) {
            ImGui::SetClipboardText(entry.name.c_str());
          }

          if (wad.fileSource && ImGui::MenuItem(ICON_SF_SQUARE_AND_ARROW_DOWN " Extract File")) {
            std::string savePath = SystemSaveFileDialog(entry.name);
            if (!savePath.empty()) {
                std::vector<uint8_t> dumpData(entry.size);
                wad.fileSource->Seek(entry.offset, 0);
                wad.fileSource->Read(dumpData.data(), entry.size);
                if (!dumpData.empty()) {
                    std::ofstream out(savePath, std::ios::binary);
                    if (out.is_open()) {
                        out.write(reinterpret_cast<const char*>(dumpData.data()), dumpData.size());
                        out.close();
                        LOG_INFO("Extracted %s to %s", entry.name.c_str(), savePath.c_str());
                    } else {
                        LOG_ERR("Failed to open path for writing: %s", savePath.c_str());
                    }
                }
            }
          }

          ImGui::EndPopup();
        }

        ImGui::PopID();
        idx++;

        // Render children
        if (node_open && has_children) {
          for (auto &child : entry.children) {
            renderEntryTree(child, idx);
          }
          ImGui::TreePop();
        }
      };

      for (auto &entry : wad.entries) {
        renderEntryTree(entry, entryIdx);
      }

      ImGui::TreePop();
    }
    ImGui::PopID();
  }

  ImGui::End();
}