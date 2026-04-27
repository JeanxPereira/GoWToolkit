#include "ui/PakBrowser.h"
#include "UIHelpers.h"
#include "core/AssetDatabase.h"
#include "core/types/TypeRegistry.h"
#include "fonts/SFSymbols.h"
#include "imgui.h"
#include "ui/AppContext.h"
#include "ui/ViewerRegistry.h"
#include "ui/viewers/DocumentWindow.h"
#include <fstream>
#include "core/Logger.h"


void PakBrowser::draw(AppContext &ctx) {
  if (!visible)
    return;

  ImGui::Begin("PAK Browser", &visible);

  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint("##pak_filter", "Filter...", m_filter,
                           sizeof(m_filter));

  ImGui::Separator();

  auto &db = ctx.db;

  for (size_t pi = 0; pi < db.paks.size(); pi++) {
    auto &pak = db.paks[pi];
    bool open =
        ImGui::TreeNodeEx(pak.filename.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 16);
    ImGui::PushID((int)pi);
    if (ImGui::SmallButton(ICON_SF_XMARK)) {
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

      const char *icon =
          IconForType(GOW::GameVersion::GOW2, entry.typeId, entry.schemaType);
      bool is_selected = (&entry == ctx.selected);

      ImGuiTreeNodeFlags flags =
          ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth |
          (is_selected ? ImGuiTreeNodeFlags_Selected : 0);

      ImGui::PushStyleColor(
          ImGuiCol_Text,
          ColorForType(GOW::GameVersion::GOW2, entry.typeId, entry.schemaType));
      ImGui::TreeNodeEx((std::string(icon) + " " + entry.name).c_str(), flags);
      ImGui::PopStyleColor();
      ImGui::TreePop();
      
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

      if (ImGui::IsItemClicked()) {
        ctx.selected = &entry;
      }

      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Pak: %s", entry.wadName.c_str());
        ImGui::Text("Offset: 0x%08X", entry.offset);
        ImGui::Text("Size: %s", FormatBytes(entry.size).c_str());
        ImGui::EndTooltip();
      }

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        // WAD files and unknown types → open as WAD browser
        if (entry.typeId == GOW::TypeId::WadFile ||
            entry.typeId == GOW::TypeId::Unknown) {
          db.LoadWadFromPakEntry(&entry, pak);
          ImGui::SetWindowFocus("WAD Browser");
        } else {
          // Try to resolve a handler by TypeId (works for file-level types)
          auto *handler = GOW::TypeRegistry::Get().Resolve(entry.typeId);
          bool canOpen =
              handler != nullptr ||
              ctx.viewerRegistry.CanHandle(GOW::GameVersion::GOW2, entry.typeId,
                                           entry.schemaType);

          if (canOpen) {
            auto fileHandle = db.OpenPakEntryAsFile(&entry, pak);
            if (fileHandle) {
              OpenWad tempWad;
              tempWad.filename = entry.name;
              tempWad.fullPath = pak.fullPath;
              tempWad.profile = pak.profile;
              tempWad.fileSource = fileHandle;

              ParsedEntry fileEntry = entry;
              fileEntry.offset = 0;
              tempWad.entries.push_back(fileEntry);

              std::shared_ptr<GOW::IDocumentContent> viewer;
              if (handler)
                viewer = handler->CreateViewer(fileEntry, tempWad);
              if (!viewer)
                viewer = ctx.viewerRegistry.Open(fileEntry, tempWad);
              if (viewer)
                ctx.documentWindow.AddTab(viewer);
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
