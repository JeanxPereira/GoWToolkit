#include "AnimCurveView.h"

#include "core/AssetDatabase.h"
#include "core/Events.h"
#include "rendering/AnimationPlayer.h"
#include "ui/ActiveAnimation.h"
#include "imgui.h"
#include "implot.h"
#include <set>
#include <algorithm>
#include <vector>

AnimCurveView::AnimCurveView() {
    EventAnimationLoaded::subscribe(this, [this](std::shared_ptr<Onyx::AnimationData> data) {
        m_animData = std::move(data);
        m_selectedGroup = 0;
        m_selectedAct = 0;
        m_selectedState = 0;
        m_selectedJoint = 0;
    });
}

AnimCurveView::~AnimCurveView() {
    EventAnimationLoaded::unsubscribe(this);
}

// Helper: extract unique joint IDs from a substream's sample keys
static std::set<int> GetJointIds(const Onyx::AnimSubstream& stream) {
    std::set<int> ids;
    for (auto& [key, _] : stream.samples) {
        if (key < 0) continue; // Skip sentinel keys
        ids.insert(key / 4);
    }
    return ids;
}

// Helper: plot a single substream component
static void PlotStreamComponent(const Onyx::AnimSubstream& stream, int jointId,
                                 int coordIdx, const char* label, int offset) {
    int key = jointId * 4 + coordIdx;
    auto it = stream.samples.find(key);
    if (it == stream.samples.end() || it->second.empty()) return;

    const auto& samples = it->second;
    std::vector<double> xs(samples.size());
    std::vector<double> ys(samples.size());

    for (size_t i = 0; i < samples.size(); i++) {
        xs[i] = (double)(i + offset);
        ys[i] = (double)samples[i];
    }

    ImPlot::PlotLine(label, xs.data(), ys.data(), (int)samples.size());
}

void AnimCurveView::Draw() {
    if (!visible) return;

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Anim Curves", &visible)) {
        ImGui::End();
        return;
    }

    if (!m_animData || m_animData->groups.empty()) {
        ImGui::TextDisabled("No animation data loaded.\n"
                            "Open a model with animation to view curves.");
        ImGui::End();
        return;
    }

    const auto* animData = m_animData.get();

    // ── Selectors ───────────────────────────────────────────────────────
    ImGui::PushItemWidth(150);

    // Group selector
    m_selectedGroup = std::clamp(m_selectedGroup, 0, (int)animData->groups.size() - 1);
    if (ImGui::BeginCombo("Group", animData->groups[m_selectedGroup].name.c_str())) {
        for (int i = 0; i < (int)animData->groups.size(); i++) {
            bool sel = (i == m_selectedGroup);
            if (ImGui::Selectable(animData->groups[i].name.c_str(), sel))
                m_selectedGroup = i;
        }
        ImGui::EndCombo();
    }

    auto& group = animData->groups[m_selectedGroup];

    ImGui::SameLine();

    // Act selector
    m_selectedAct = std::clamp(m_selectedAct, 0, std::max(0, (int)group.acts.size() - 1));
    if (ImGui::BeginCombo("Act", group.acts.empty() ? "---" :
            group.acts[m_selectedAct].name.c_str())) {
        for (int i = 0; i < (int)group.acts.size(); i++) {
            bool sel = (i == m_selectedAct);
            if (ImGui::Selectable(group.acts[i].name.c_str(), sel))
                m_selectedAct = i;
        }
        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    if (group.acts.empty()) {
        ImGui::TextDisabled("No acts in this group.");
        ImGui::End();
        return;
    }

    auto& act = group.acts[m_selectedAct];

    // Find skinning state descriptor
    int skinIdx = animData->FindSkinningTypeIndex();
    if (skinIdx < 0 || skinIdx >= (int)act.stateDescrs.size() ||
        act.stateDescrs[skinIdx].skinningStates.empty()) {
        ImGui::TextDisabled("No skinning data in this act.");
        ImGui::End();
        return;
    }

    auto& stateDescr = act.stateDescrs[skinIdx];

    // State selector (multiple skinning states possible)
    ImGui::PushItemWidth(100);
    m_selectedState = std::clamp(m_selectedState, 0,
                                  (int)stateDescr.skinningStates.size() - 1);
    if (stateDescr.skinningStates.size() > 1) {
        ImGui::SliderInt("State", &m_selectedState, 0,
                         (int)stateDescr.skinningStates.size() - 1);
    }

    auto& skinState = stateDescr.skinningStates[m_selectedState];

    // Gather unique joint IDs from rotation and position streams
    auto rotJoints = GetJointIds(skinState.rotationStream);
    auto posJoints = GetJointIds(skinState.positionStream);
    std::set<int> allJoints;
    allJoints.insert(rotJoints.begin(), rotJoints.end());
    allJoints.insert(posJoints.begin(), posJoints.end());

    std::vector<int> jointList(allJoints.begin(), allJoints.end());

    if (jointList.empty()) {
        ImGui::TextDisabled("No joint data in this state.");
        ImGui::PopItemWidth();
        ImGui::End();
        return;
    }

    // Joint selector
    m_selectedJoint = std::clamp(m_selectedJoint, 0,
                                  std::max(0, (int)jointList.size() - 1));
    char jointLabel[32];
    snprintf(jointLabel, sizeof(jointLabel), "Joint %d", jointList[m_selectedJoint]);
    if (ImGui::BeginCombo("Joint", jointLabel)) {
        for (int i = 0; i < (int)jointList.size(); i++) {
            char lbl[32];
            snprintf(lbl, sizeof(lbl), "Joint %d", jointList[i]);
            if (ImGui::Selectable(lbl, i == m_selectedJoint))
                m_selectedJoint = i;
        }
        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    int jointId = jointList[m_selectedJoint];

    // Toggles
    ImGui::SameLine();
    ImGui::Checkbox("Rotation", &m_showRotation);
    ImGui::SameLine();
    ImGui::Checkbox("Position", &m_showPosition);

    ImGui::Spacing();

    // ── Act info ────────────────────────────────────────────────────────
    ImGui::TextDisabled("Duration: %.3fs  |  Joints: %zu  |  Frame Time: %.4fs",
                        act.duration, jointList.size(), stateDescr.frameTime);

    // Cached playhead frame — drawn as a vertical line on each plot so the
    // user can match what they see in the viewport to where the value lives
    // on the curve.
    Onyx::AnimationPlayer* activePlayer = Onyx::UI::GetActiveAnimationPlayer();
    double playheadFrame = -1.0;
    if (activePlayer && activePlayer->GetFrameCount() > 0) {
        playheadFrame = (double)activePlayer->GetCurrentFrame();
    }

    auto drawPlayhead = [&](double frame) {
        if (frame < 0.0) return;
        // PlotInfLines draws a vertical line at the given X across the plot.
        // It's the documented ImPlot API for this and stays inside the plot
        // clipping rect without touching draw-list internals.
        double xs[1] = { frame };
        ImPlot::PlotInfLines("##playhead", xs, 1);
    };

    // ── Rotation Plot ───────────────────────────────────────────────────
    int offset = skinState.rotationStream.manager.offset;

    if (m_showRotation) {
        if (ImPlot::BeginPlot("Rotation", ImVec2(-1, 200))) {
            ImPlot::SetupAxes("Frame", "Value (Q.14)");
            ImPlot::SetupLegend(ImPlotLocation_NorthEast);

            PlotStreamComponent(skinState.rotationStream, jointId, 0, "Rot X", offset);
            PlotStreamComponent(skinState.rotationStream, jointId, 1, "Rot Y", offset);
            PlotStreamComponent(skinState.rotationStream, jointId, 2, "Rot Z", offset);
            PlotStreamComponent(skinState.rotationStream, jointId, 3, "Rot W", offset);
            drawPlayhead(playheadFrame);

            ImPlot::EndPlot();
        }
    }

    // ── Position Plot ───────────────────────────────────────────────────
    if (m_showPosition) {
        offset = skinState.positionStream.manager.offset;
        if (ImPlot::BeginPlot("Position", ImVec2(-1, 200))) {
            ImPlot::SetupAxes("Frame", "Value (world units)");
            ImPlot::SetupLegend(ImPlotLocation_NorthEast);

            PlotStreamComponent(skinState.positionStream, jointId, 0, "Pos X", offset);
            PlotStreamComponent(skinState.positionStream, jointId, 1, "Pos Y", offset);
            PlotStreamComponent(skinState.positionStream, jointId, 2, "Pos Z", offset);
            drawPlayhead(playheadFrame);

            ImPlot::EndPlot();
        }
    }

    ImGui::End();
}
