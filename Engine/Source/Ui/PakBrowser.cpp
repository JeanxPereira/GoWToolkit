#include "Ui/PakBrowser.h"
#include "UIHelpers.h"
#include "Core/AssetDatabase.h"
#include "Core/ToolkitApi.h"
#include "Fonts/SFSymbols.h"
#include "imgui.h"
#include "Ui/ViewerRegistry.h"
#include "Ui/Widgets.h"
#include "Ui/Viewers/DocumentWindow.h"
#include <fstream>
#include "Core/Logger.h"


void PakBrowser::Draw() {
  if (!visible)
    return;

  ImGui::Begin("PAK Browser", &visible);

  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint("##pak_filter", "Filter...", m_filter,
                           sizeof(m_filter));

  ImGui::Separator();

  auto &db = Onyx::Api::Database();

  for (size_t pi = 0; pi < db.paks.size(); pi++) {
    auto &pak = db.paks[pi];
    bool open =
        ImGui::TreeNodeEx(pak.filename.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    ImGui::PushID((int)pi);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight());
    Onyx::UI::Widgets::IconButtonOpts closeOpts;
    closeOpts.tooltip = "Close PAK";
    if (Onyx::UI::Widgets::IconButton("pak_close", ICON_SF_XMARK, closeOpts)) {
      db.ClosePak(pi);
      ImGui::PopID();
      if (open)
        ImGui::TreePop();
      continue;
    }
    ImGui::PopID();

    if (!open)
      continue;

    for (auto &entry : pak.entries) {
      if (m_filter[0] && !MatchesFilter(entry.name, m_filter))
        continue;

      const char *icon = IconForType(entry.typeId);
      const ImVec4 color = ColorForType(entry.typeId);
      bool is_selected = (&entry == Onyx::Api::GetSelected());

      ImGuiTreeNodeFlags flags =
          ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
          ImGuiTreeNodeFlags_SpanFullWidth;

      // Stable per-row id via entry offset (unique within a PAK).
      char rowId[24];
      snprintf(rowId, sizeof(rowId), "%u", entry.offset);
      Onyx::UI::Widgets::ColoredTreeNode(rowId, entry.name.c_str(), icon, color, flags, is_selected);
      
      ImGui::PushID((int)entry.offset);
      if (ImGui::BeginPopupContextItem()) {
          if (ImGui::MenuItem(ICON_SF_DOCUMENT_ON_DOCUMENT " Copy Name")) {
              ImGui::SetClipboardText(entry.name.c_str());
          }
          if (ImGui::MenuItem(ICON_SF_SQUARE_AND_ARROW_DOWN " Extract File")) {
              std::string savePath = SystemSaveFileDialog(entry.name);
              if (!savePath.empty()) {
                  auto fileHandle = db.OpenPakEntryAsFile(&entry, pak);
                  if (fileHandle) {
                      std::vector<uint8_t> dumpData(entry.size);
                      fileHandle->Seek(0, 0);
                      fileHandle->Read(dumpData.data(), entry.size);
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
                  } else {
                      LOG_ERR("Failed to open pak entry for extraction.", "");
                  }
              }
          }
          ImGui::EndPopup();
      }
      ImGui::PopID();

      // â”€â”€ Selection (single click) â€” via Api::SetSelected â”€â”€
      if (ImGui::IsItemClicked()) {
        Onyx::Api::SetSelected(&entry, &pak);
      }

      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Pak: %s", entry.wadName.c_str());
        ImGui::Text("Offset: 0x%08X", entry.offset);
        ImGui::Text("Size: %s", FormatBytes(entry.size).c_str());
        ImGui::EndTooltip();
      }

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        // WAD files and unknown types â†’ open as WAD browser. The decision of
        // what counts as an openable container is owned by the active profile.
        if (pak.profile && pak.profile->IsContainerEntry(entry)) {
          db.LoadWadFromPakEntry(&entry, pak);
          ImGui::SetWindowFocus("WAD Browser");
        } else {
          if (Onyx::Api::Viewers().CanHandle(entry.typeId)) {
            auto fileHandle = db.OpenPakEntryAsFile(&entry, pak);
            if (fileHandle) {
              AssetContainer tempWad;
              tempWad.filename = entry.name;
              tempWad.fullPath = pak.fullPath;
              tempWad.profile = pak.profile;
              tempWad.fileSource = fileHandle;

              AssetEntry fileEntry = entry;
              fileEntry.offset = 0;
              tempWad.entries.push_back(fileEntry);

              auto viewer = Onyx::Api::Viewers().Open(fileEntry, tempWad);
              if (viewer)
                Onyx::Api::Documents().AddTab(viewer);
            }
          } else {
            db.LoadWadFromPakEntry(&entry, pak);
            ImGui::SetWindowFocus("WAD Browser");
          }
        }
      }
    }
    ImGui::TreePop();
  }

  ImGui::End();
}
