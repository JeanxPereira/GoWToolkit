#include "ui/StatusBar.h"
#include "ui/AppContext.h"
#include "core/Logger.h"
#include "core/AssetDatabase.h"
#include "imgui.h"

void StatusBar::draw(AppContext& ctx) {
    if (!visible) return;
    ImGui::Begin("Log", &visible);

    auto loadState = ctx.db.m_loadState.load();
    if (loadState == AssetDatabase::LoadState::LoadingWad || 
        loadState == AssetDatabase::LoadState::LoadingIsoPak) {
        ImGui::TextUnformatted(ctx.db.m_loadMessage.c_str());
        ImGui::SameLine();
        
        // Use an indeterminate-like continuous progress if loadProgress stays at 0.0f
        float progress = ctx.db.m_loadProgress.load();
        if (progress == 0.0f) progress = (float)ImGui::GetTime() * 0.5f; 
        else progress = progress; 
        // ProgressBar wraps around if progress > 1.0f but we just want an animated bar.
        // Let's use ImGui animated indeterminate bar if we wrap it, or just use modulo.
        float displayProgress = std::fmod(progress, 1.0f);
        
        ImGui::ProgressBar(displayProgress, ImVec2(-1.0f, 0.0f), "");
        ImGui::Separator();
    }

    if (ImGui::Button("Clear")) {
        GOW::Logger::Get().Clear();
    }
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy to Clipboard");
    ImGui::Separator();

    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (copy) ImGui::LogToClipboard();

    auto entries = GOW::Logger::Get().GetEntries();

    // Store scroll state before drawing logic
    bool atBottom = (ImGui::GetScrollY() >= ImGui::GetScrollMaxY());

    for (int i = 0; i < (int)entries.size(); i++) {
        const auto& entry = entries[i];

        // Skip debug messages in UI
        if (entry.level == GOW::LogLevel::Debug) continue;

        ImVec4 color;
        if (entry.level == GOW::LogLevel::Error) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        else if (entry.level == GOW::LogLevel::Warning) color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f);
        else color = ImGui::GetStyleColorVec4(ImGuiCol_Text);

        ImGui::PushID(i);

        char label[64];
        snprintf(label, sizeof(label), "##log%d", i);

        bool isSelected = (selectedLog == i);
        ImVec2 cursorLine = ImGui::GetCursorPos();

        if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
            selectedLog = i;
        }

        if (ImGui::BeginPopupContextItem()) {
            selectedLog = i;
            if (ImGui::MenuItem("Copy Line")) {
                std::string lineStr = "[" + entry.time + "] " + entry.message;
                ImGui::SetClipboardText(lineStr.c_str());
            }
            if (ImGui::MenuItem("Copy All")) {
                std::string allLogs;
                for (const auto& e : entries) {
                    if (e.level == GOW::LogLevel::Debug) continue;
                    allLogs += "[" + e.time + "] " + e.message + "\n";
                }
                ImGui::SetClipboardText(allLogs.c_str());
            }
            ImGui::EndPopup();
        }

        // Draw actual text
        ImGui::SetCursorPos(cursorLine);
        ImGui::TextDisabled("[%s]", entry.time.c_str());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(entry.message.c_str());
        ImGui::PopStyleColor();

        ImGui::PopID();
    }

    if (copy) ImGui::LogFinish();

    // Auto-scroll when at bottom
    if (atBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

void StatusBar::SetMessage(const char* msg) {
    LOG_INFO("%s", msg);
}
