#pragma once
#include "Ui/IPanel.h"
#include <vector>
#include <string>
#include <cstdint>

namespace Onyx::Viewers {

/// Panel that shows WAD file statistics using ImPlot charts.
/// Displays: asset type distribution, size breakdown, and top entries.
class WadStatsView : public Onyx::App::IPanel {
public:
    void Draw() override;
    std::string_view getName() const override { return "WAD Stats"; }

private:
    struct TypeStat {
        std::string label;
        uint32_t    count = 0;
        uint64_t    totalSize = 0;
    };

    void computeStats();

    std::vector<TypeStat> m_stats;
    size_t                m_lastWadCount = 0;
    bool                  m_dirty = true;
};

} // namespace Onyx::Viewers
