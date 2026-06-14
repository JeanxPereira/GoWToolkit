#include "Ui/StatusBar.h"

#include "Core/Logger.h"
#include "Core/AssetDatabase.h"
#include "Core/TaskManager.h"
#include "Core/Events.h"
#include "Core/Domain/Wad.h"
#include "imgui.h"

#include <cmath>

namespace Onyx::App {

StatusBar::StatusBar() {
    EventWadOpened::subscribe(this, [this](AssetContainer* wad) {
        if (wad) {
            std::string msg = "Loaded: " + wad->filename;
            SetMessage(msg.c_str());
        }
    });

    EventPakOpened::subscribe(this, [this](AssetContainer* pak) {
        if (pak) {
            std::string msg = "Loaded PAK: " + pak->filename;
            SetMessage(msg.c_str());
        }
    });

    EventAllClosed::subscribe(this, [this]() {
        SetMessage("All files closed.");
    });
}

StatusBar::~StatusBar() {
    EventWadOpened::unsubscribe(this);
    EventPakOpened::unsubscribe(this);
    EventAllClosed::unsubscribe(this);
}

void StatusBar::Draw() {
    if (!visible) return;
    ImGui::Begin("Log", &visible);

    // ── TaskManager Progress (new system) ────────────────────────────────────
    auto& tasks = Onyx::Services::TaskManager::getRunningTasks();
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


    // ── Background tasks indicator ────────────────────────────────────────────
    size_t bgCount = Onyx::Services::TaskManager::getRunningBackgroundTaskCount();
    if (bgCount > 0) {
        if (hasVisibleTasks) ImGui::Separator();
        ImGui::TextDisabled("Background tasks: %zu", bgCount);
        hasVisibleTasks = true;
    }

    if (hasVisibleTasks) ImGui::Separator();

    // ── Log viewer ────────────────────────────────────────────────────────────
    if (ImGui::Button("Clear")) {
        Onyx::Services::Logger::Get().Clear();
    }
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy to Clipboard");
    ImGui::Separator();

    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (copy) ImGui::LogToClipboard();

    auto entries = Onyx::Services::Logger::Get().GetEntries();

    // Store scroll state before drawing logic
    bool atBottom = (ImGui::GetScrollY() >= ImGui::GetScrollMaxY());

    for (int i = 0; i < (int)entries.size(); i++) {
        const auto& entry = entries[i];

        // Skip debug messages in UI
        if (entry.level == Onyx::Services::LogLevel::Debug) continue;

        ImVec4 color;
        if (entry.level == Onyx::Services::LogLevel::Error) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        else if (entry.level == Onyx::Services::LogLevel::Warning) color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f);
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
                    if (e.level == Onyx::Services::LogLevel::Debug) continue;
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

} // namespace Onyx::App
