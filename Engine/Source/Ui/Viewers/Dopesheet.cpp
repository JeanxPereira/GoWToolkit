#include "Dopesheet.h"

#include <Onyx/Services/Events.h>
#include <Onyx/Rendering/AnimationPlayer.h>
#include "Ui/ActiveAnimation.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <vector>


namespace Onyx::Viewers {
Dopesheet::Dopesheet() {
    EventAnimationLoaded::subscribe(this, [this](std::shared_ptr<Onyx::Parsers::AnimationData> data) {
        m_animData = std::move(data);
        m_selectedGroup = 0;
        m_selectedAct = 0;
        m_selectedState = 0;
    });
}

Dopesheet::~Dopesheet() {
    EventAnimationLoaded::unsubscribe(this);
}

namespace {

void GatherJointIds(const Onyx::Parsers::AnimSubstream& s, std::set<int>& out) {
    for (auto& [key, _] : s.samples) {
        if (key < 0) continue;
        out.insert(key / 4);
    }
}

void GatherFromState(const Onyx::Parsers::SkinningState& st, std::set<int>& outJoints,
                     bool rotation, bool position) {
    if (rotation) {
        GatherJointIds(st.rotationStream, outJoints);
        for (auto& s : st.rotationSubStreamsAdd)   GatherJointIds(s, outJoints);
        for (auto& s : st.rotationSubStreamsRough) GatherJointIds(s, outJoints);
    }
    if (position) {
        GatherJointIds(st.positionStream, outJoints);
        for (auto& s : st.positionSubStreamsAdd)   GatherJointIds(s, outJoints);
        for (auto& s : st.positionSubStreamsRough) GatherJointIds(s, outJoints);
    }
}

// Return the list of (frameOffset, frameCount) ranges where this joint has
// samples in the given substream. The dopesheet draws a tick at the start
// of each range so dense channels stay legible.
struct ChannelRange {
    int start;
    int count;
};

void CollectRanges(const Onyx::Parsers::AnimSubstream& s, int jointId,
                   std::vector<ChannelRange>& out) {
    bool any = false;
    for (int coord = 0; coord < 4; ++coord) {
        int key = jointId * 4 + coord;
        auto it = s.samples.find(key);
        if (it == s.samples.end() || it->second.empty()) continue;
        any = true;
    }
    if (!any) return;
    out.push_back({ (int)s.manager.offset, (int)s.manager.count });
}

} // namespace

void Dopesheet::Draw() {
    if (!visible) return;

    ImGui::SetNextWindowSize(ImVec2(720, 460), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Dopesheet", &visible)) {
        ImGui::End();
        return;
    }

    if (!m_animData || m_animData->groups.empty()) {
        ImGui::TextDisabled("No animation data loaded.");
        ImGui::End();
        return;
    }

    const auto* animData = m_animData.get();

    // â”€â”€ Selectors â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImGui::PushItemWidth(160);
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
    m_selectedAct = std::clamp(m_selectedAct, 0, std::max(0, (int)group.acts.size() - 1));
    if (ImGui::BeginCombo("Act", group.acts.empty() ? "---" : group.acts[m_selectedAct].name.c_str())) {
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

    int skinIdx = animData->FindSkinningTypeIndex();
    if (skinIdx < 0 || skinIdx >= (int)act.stateDescrs.size() ||
        act.stateDescrs[skinIdx].skinningStates.empty()) {
        ImGui::TextDisabled("No skinning data in this act.");
        ImGui::End();
        return;
    }
    auto& sd = act.stateDescrs[skinIdx];

    ImGui::SameLine();
    ImGui::Checkbox("Rot", &m_showRotation);
    ImGui::SameLine();
    ImGui::Checkbox("Pos", &m_showPosition);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::DragFloat("Zoom", &m_zoom, 0.5f, 2.0f, 64.0f, "%.1f px/frame");

    // Compute frame count: prefer the active player (already baked), else
    // fall back to ceil(duration / frameTime).
    int frameCount = 0;
    float frameTime = sd.frameTime > 0 ? sd.frameTime : (1.0f / 30.0f);
    Onyx::Rendering::AnimationPlayer* player = Onyx::App::GetActiveAnimationPlayer();
    if (player && player->GetFrameCount() > 0 &&
        player->GetCurrentGroupIndex() == m_selectedGroup &&
        player->GetCurrentActIndex() == m_selectedAct) {
        frameCount = player->GetFrameCount();
        frameTime = player->GetFrameTime();
    } else if (act.duration > 0.0f) {
        frameCount = (int)std::ceil(act.duration / frameTime) + 1;
    } else {
        frameCount = 1;
    }
    int lastFrame = std::max(0, frameCount - 1);

    // Joint list
    std::set<int> jointSet;
    for (auto& ss : sd.skinningStates) {
        GatherFromState(ss, jointSet, m_showRotation, m_showPosition);
    }
    std::vector<int> joints(jointSet.begin(), jointSet.end());
    if (joints.empty()) {
        ImGui::TextDisabled("No animated joints with current filters.");
        ImGui::End();
        return;
    }

    ImGui::Text("Joints: %zu  Frames: %d  FrameTime: %.4fs", joints.size(),
                frameCount, frameTime);

    ImGui::Separator();

    // â”€â”€ Grid drawing â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float gridWidth = m_zoom * (float)frameCount;
    float gridHeight = m_rowHeight * (float)joints.size();
    ImVec2 totalSize(m_jointLabelWidth + gridWidth + 16.0f, gridHeight + 24.0f);

    ImGui::BeginChild("##dopegrid", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::Dummy(totalSize);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 colBg     = IM_COL32(28, 28, 32, 255);
    ImU32 colRowAlt = IM_COL32(40, 40, 46, 255);
    ImU32 colSep    = IM_COL32(70, 70, 76, 200);
    ImU32 colTick10 = IM_COL32(120, 120, 120, 120);
    ImU32 colTick1  = IM_COL32(80, 80, 80, 80);
    ImU32 colRotKey = IM_COL32(120, 200, 255, 230);
    ImU32 colPosKey = IM_COL32(255, 180, 90, 230);
    ImU32 colPlay   = IM_COL32(255, 100, 100, 220);

    float xStart = cursor.x + m_jointLabelWidth;
    float yStart = cursor.y + 20.0f;

    // Frame header
    for (int f = 0; f <= lastFrame; ++f) {
        bool tens = (f % 10) == 0;
        if (!tens && m_zoom < 8.0f) continue;
        float x = xStart + f * m_zoom;
        dl->AddLine(ImVec2(x, yStart - 4), ImVec2(x, yStart + gridHeight),
                    tens ? colTick10 : colTick1, 1.0f);
        if (tens) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", f);
            dl->AddText(ImVec2(x + 2, cursor.y + 2), colTick10, buf);
        }
    }

    // Row backgrounds + joint labels
    for (size_t r = 0; r < joints.size(); ++r) {
        float y0 = yStart + r * m_rowHeight;
        float y1 = y0 + m_rowHeight;
        if (r & 1) {
            dl->AddRectFilled(ImVec2(cursor.x, y0),
                              ImVec2(cursor.x + totalSize.x, y1), colRowAlt);
        }
        char lbl[24];
        snprintf(lbl, sizeof(lbl), "joint[%d]", joints[r]);
        dl->AddText(ImVec2(cursor.x + 4, y0 + 1),
                    ImGui::GetColorU32(ImGuiCol_Text), lbl);
        dl->AddLine(ImVec2(cursor.x + m_jointLabelWidth, y0),
                    ImVec2(cursor.x + m_jointLabelWidth, y1), colSep);
    }
    (void)colBg;

    // Channel ticks
    for (size_t r = 0; r < joints.size(); ++r) {
        float yMid = yStart + r * m_rowHeight + m_rowHeight * 0.5f;
        std::vector<ChannelRange> rotRanges, posRanges;
        for (auto& ss : sd.skinningStates) {
            if (m_showRotation) {
                CollectRanges(ss.rotationStream, joints[r], rotRanges);
                for (auto& s : ss.rotationSubStreamsAdd)
                    CollectRanges(s, joints[r], rotRanges);
                for (auto& s : ss.rotationSubStreamsRough)
                    CollectRanges(s, joints[r], rotRanges);
            }
            if (m_showPosition) {
                CollectRanges(ss.positionStream, joints[r], posRanges);
                for (auto& s : ss.positionSubStreamsAdd)
                    CollectRanges(s, joints[r], posRanges);
                for (auto& s : ss.positionSubStreamsRough)
                    CollectRanges(s, joints[r], posRanges);
            }
        }
        auto plotRanges = [&](const std::vector<ChannelRange>& v, ImU32 col, float yOff) {
            for (auto& rr : v) {
                if (rr.count <= 0) continue;
                float x0 = xStart + rr.start * m_zoom;
                float x1 = xStart + (rr.start + rr.count - 1) * m_zoom;
                dl->AddLine(ImVec2(x0, yMid + yOff), ImVec2(x1, yMid + yOff), col, 2.0f);
                dl->AddCircleFilled(ImVec2(x0, yMid + yOff), 2.0f, col);
                dl->AddCircleFilled(ImVec2(x1, yMid + yOff), 2.0f, col);
            }
        };
        plotRanges(rotRanges, colRotKey, -3.0f);
        plotRanges(posRanges, colPosKey, +3.0f);
    }

    // Playhead + interaction
    int playFrame = -1;
    if (player && player->GetCurrentGroupIndex() == m_selectedGroup &&
        player->GetCurrentActIndex() == m_selectedAct) {
        playFrame = player->GetCurrentFrame();
    }
    if (playFrame >= 0) {
        float x = xStart + playFrame * m_zoom;
        dl->AddLine(ImVec2(x, yStart - 4),
                    ImVec2(x, yStart + gridHeight), colPlay, 2.0f);
    }

    // Mouse: click in grid area scrubs the player
    if (ImGui::IsWindowHovered()) {
        ImVec2 mp = ImGui::GetMousePos();
        if (mp.x >= xStart && mp.x <= xStart + gridWidth &&
            mp.y >= yStart && mp.y <= yStart + gridHeight) {
            int hoverFrame = (int)std::round((mp.x - xStart) / m_zoom);
            hoverFrame = std::clamp(hoverFrame, 0, lastFrame);
            if (ImGui::BeginTooltip()) {
                ImGui::Text("Frame %d  Time %.3fs", hoverFrame, hoverFrame * frameTime);
                ImGui::EndTooltip();
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && player &&
                player->GetCurrentGroupIndex() == m_selectedGroup &&
                player->GetCurrentActIndex() == m_selectedAct) {
                player->SetFrame(hoverFrame);
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
}


} // namespace Onyx::Viewers
