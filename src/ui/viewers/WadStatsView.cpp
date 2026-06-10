#include "WadStatsView.h"

#include "core/AssetDatabase.h"
#include "core/Events.h"
#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <numeric>
#include <map>

#include "core/ToolkitApi.h"
#include "UIHelpers.h"

void WadStatsView::computeStats() {
    m_stats.clear();

    std::map<std::string, TypeStat> byType;

    for (auto& wad : Onyx::Api::Database().wads) {
        for (auto& entry : wad.entries) {
            std::string label = TypeName(entry.typeId);
            auto& stat = byType[label];
            stat.label = label;
            stat.count++;
            stat.totalSize += entry.size;
        }
    }

    m_stats.reserve(byType.size());
    for (auto& [k, v] : byType)
        m_stats.push_back(std::move(v));

    // Sort by count descending
    std::sort(m_stats.begin(), m_stats.end(), [](const TypeStat& a, const TypeStat& b) {
        return a.count > b.count;
    });

    m_lastWadCount = Onyx::Api::Database().wads.size();
    m_dirty = false;
}

void WadStatsView::Draw() {
    if (!visible) return;

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("WAD Stats", &visible)) {
        ImGui::End();
        return;
    }

    if (Onyx::Api::Database().wads.empty()) {
        ImGui::TextDisabled("No WAD files loaded.");
        ImGui::End();
        return;
    }

    // Recompute stats if WADs changed
    if (m_dirty || Onyx::Api::Database().wads.size() != m_lastWadCount)
        computeStats();

    // ── Summary Header ──────────────────────────────────────────────────
    uint32_t totalEntries = 0;
    uint64_t totalSize    = 0;
    for (auto& s : m_stats) {
        totalEntries += s.count;
        totalSize    += s.totalSize;
    }

    ImGui::Text("Total: %u entries  |  %zu types  |  %.2f MB",
                totalEntries, m_stats.size(), totalSize / (1024.0 * 1024.0));
    ImGui::Separator();
    ImGui::Spacing();

    // ── Pie Chart: Distribution by Type ─────────────────────────────────
    if (ImGui::CollapsingHeader("Type Distribution", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Prepare data arrays for ImPlot
        const int maxSlices = std::min((int)m_stats.size(), 12);
        std::vector<double>      values(maxSlices);
        std::vector<const char*> labels(maxSlices);

        for (int i = 0; i < maxSlices; i++) {
            values[i] = (double)m_stats[i].count;
            labels[i] = m_stats[i].label.c_str();
        }

        if (ImPlot::BeginPlot("##TypePie", ImVec2(-1, 250),
                              ImPlotFlags_NoMouseText | ImPlotFlags_Equal)) {
            ImPlot::SetupAxes(nullptr, nullptr,
                              ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
            ImPlot::PlotPieChart(labels.data(), values.data(), maxSlices,
                                 0.5, 0.5, 0.4, "%.0f", 90.0);
            ImPlot::EndPlot();
        }
    }

    // ── Bar Chart: Size per Type ────────────────────────────────────────
    if (ImGui::CollapsingHeader("Size by Type (MB)", ImGuiTreeNodeFlags_DefaultOpen)) {
        const int maxBars = std::min((int)m_stats.size(), 15);
        std::vector<double>      sizes(maxBars);
        std::vector<const char*> labels(maxBars);
        std::vector<double>      positions(maxBars);

        for (int i = 0; i < maxBars; i++) {
            sizes[i]     = m_stats[i].totalSize / (1024.0 * 1024.0);
            labels[i]    = m_stats[i].label.c_str();
            positions[i] = (double)i;
        }

        if (ImPlot::BeginPlot("##SizeBars", ImVec2(-1, 250))) {
            ImPlot::SetupAxes("Type", "Size (MB)");
            ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), maxBars, labels.data());
            ImPlot::PlotBars("Size", positions.data(), sizes.data(), maxBars, 0.67);
            ImPlot::EndPlot();
        }
    }

    // ── Table: Full Breakdown ───────────────────────────────────────────
    if (ImGui::CollapsingHeader("Detailed Breakdown")) {
        if (ImGui::BeginTable("##StatsTable", 4,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY,
                ImVec2(0, 200))) {
            ImGui::TableSetupColumn("Type",  ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("Size",  ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("%",     ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& s : m_stats) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(s.label.c_str());
                ImGui::TableNextColumn(); ImGui::Text("%u", s.count);
                ImGui::TableNextColumn();
                if (s.totalSize < 1024)
                    ImGui::Text("%llu B", (unsigned long long)s.totalSize);
                else if (s.totalSize < 1024*1024)
                    ImGui::Text("%.1f KB", s.totalSize / 1024.0);
                else
                    ImGui::Text("%.2f MB", s.totalSize / (1024.0*1024.0));
                ImGui::TableNextColumn();
                float pct = totalSize > 0 ? (float)s.totalSize / totalSize * 100.0f : 0.0f;
                ImGui::Text("%.1f", pct);
            }
            ImGui::EndTable();
        }
    }

    ImGui::End();
}
