#include "ui/StatusBar.h"
#include "ui/AppContext.h"
#include "core/Logger.h"
#include "core/AssetDatabase.h"
#include "core/TaskManager.h"
#include "imgui.h"

void StatusBar::draw(AppContext& ctx) {
    if (!visible) return;
    ImGui::Begin("Log", &visible);

    // ── TaskManager Progress (new system) ────────────────────────────
    auto& tasks = GOW::TaskManager::getRunningTasks();
    bool hasVisibleTasks = false;

    for (auto& task : tasks) {
        if (task->isFinished() || task->isBackgroundTask()) continue;
        hasVisibleTasks = true;

        ImGui::TextUnformatted(task->getName().c_str());
        ImGui::SameLine();

        uint64_t maxVal = task->getMaxValue();
        if (maxVal > 0) {
            // Determinate progress
            float progress = (float)task->getValue() / (float)maxVal;
            ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
        } else {
            // Indeterminate progress (animated)
            float t = std::fmod((float)ImGui::GetTime() * 0.5f, 1.0f);
            ImGui::ProgressBar(t, ImVec2(-1.0f, 0.0f), "");
        }
    }

    // ── Legacy AssetDatabase progress (while migration is in progress) ──
    if (!hasVisibleTasks) {
        auto loadState = ctx.db.m_loadState.load();
        if (loadState == AssetDatabase::LoadState::LoadingWad || 
            loadState == AssetDatabase::LoadState::LoadingIsoPak) {
            ImGui::TextUnformatted(ctx.db.m_loadMessage.c_str());
            ImGui::SameLine();
            
            float progress = ctx.db.m_loadProgress.load();
            if (progress == 0.0f) progress = (float)ImGui::GetTime() * 0.5f; 
            float displayProgress = std::fmod(progress, 1.0f);
            
            ImGui::ProgressBar(displayProgress, ImVec2(-1.0f, 0.0f), "");
            hasVisibleTasks = true;
        }
    }

    // ── Background tasks indicator ──────────────────────────────────
    size_t bgCount = GOW::TaskManager::getRunningBackgroundTaskCount();
    if (bgCount > 0) {
        if (hasVisibleTasks) ImGui::Separator();
        ImGui::TextDisabled("Background tasks: %zu", bgCount);
        hasVisibleTasks = true;
    }

    if (hasVisibleTasks) ImGui::Separator();

    // ── Log viewer ──────────────────────────────────────────────────
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
