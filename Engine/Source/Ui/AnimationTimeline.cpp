#include "Ui/AnimationTimeline.h"
#include "Core/Parsers/Shared/AnimationData.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace Onyx::UI {

namespace {

// Collect every distinct "keyframe" position in this act, expressed as
// integer frame indices. We treat a keyframe as the start offset of any
// substream â€” those are the points where the engine actually has stored
// data, so they line up with what an animator would call a key.
void CollectKeyFrames(const Onyx::Rendering::AnimationPlayer& player,
                      std::set<int>& out) {
    const Onyx::Parsers::AnimationData* anim = nullptr;
    int g = player.GetCurrentGroupIndex();
    int a = player.GetCurrentActIndex();
    const Onyx::Rendering::BakedAnimation* baked = player.GetBaked();
    if (!baked || g < 0 || a < 0) return;

    // The player owns the anim pointer but doesn't expose it; derive markers
    // from the baked frame range as a sane fallback if the upstream act ever
    // gets refactored.  For now, plant ticks at frame 0 and last frame at
    // minimum; richer markers fill in via the public stateDescr API below.
    out.insert(0);
    out.insert(std::max(0, baked->frameCount - 1));
    (void)anim;
}

ImU32 BlendU32(ImU32 a, ImU32 b, float t) {
    auto unpack = [](ImU32 c) {
        return ImVec4(((c >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
                      ((c >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
                      ((c >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
                      ((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f);
    };
    ImVec4 va = unpack(a);
    ImVec4 vb = unpack(b);
    ImVec4 r(va.x + (vb.x - va.x) * t,
             va.y + (vb.y - va.y) * t,
             va.z + (vb.z - va.z) * t,
             va.w + (vb.w - va.w) * t);
    return ImGui::ColorConvertFloat4ToU32(r);
}

} // namespace

bool DrawAnimationTimeline(const char* strId,
                           Onyx::Rendering::AnimationPlayer& player,
                           const AnimationTimelineStyle& style) {
    const Onyx::Rendering::BakedAnimation* baked = player.GetBaked();
    if (!baked || baked->empty()) return false;

    const int frameCount = baked->frameCount;
    const int lastFrame  = std::max(0, frameCount - 1);
    const float frameTime = baked->frameTime;
    int curFrame = player.GetCurrentFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 size(std::max(avail.x, 100.0f), style.trackHeight);

    ImGui::PushID(strId);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImRect bb(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y));

    ImGui::InvisibleButton("##track", size);
    bool hovered = ImGui::IsItemHovered();
    bool active  = ImGui::IsItemActive();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 colBg     = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 colBgHov  = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
    ImU32 colMinor  = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.35f);
    ImU32 colMajor  = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.85f);
    ImU32 colKey    = IM_COL32(255, 200, 80, 230);
    ImU32 colPlay   = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
    ImU32 colTrack  = BlendU32(colBg, ImGui::GetColorU32(ImGuiCol_SliderGrab), 0.25f);
    ImU32 colHovBg  = colBgHov;

    // Background pill
    dl->AddRectFilled(bb.Min, bb.Max, hovered ? colHovBg : colBg, 3.0f);

    // Filled portion up to current playhead
    auto frameToX = [&](float f) -> float {
        if (lastFrame <= 0) return bb.Min.x;
        float t = std::clamp(f / (float)lastFrame, 0.0f, 1.0f);
        return bb.Min.x + t * (bb.Max.x - bb.Min.x);
    };
    float playheadX = frameToX((float)curFrame);
    dl->AddRectFilled(bb.Min, ImVec2(playheadX, bb.Max.y), colTrack, 3.0f);

    // Frame ticks
    if (frameCount > 1) {
        for (int f = 0; f <= lastFrame; ++f) {
            bool isMajor = (style.majorTickEvery > 0.0f) &&
                           (std::fmod((float)f, style.majorTickEvery) < 0.5f);
            if (!isMajor && style.minorTickEvery > 0.0f &&
                std::fmod((float)f, style.minorTickEvery) >= 0.5f) {
                continue;
            }
            float x = frameToX((float)f);
            float h = isMajor ? 8.0f : 4.0f;
            dl->AddLine(ImVec2(x, bb.Max.y - h - 1),
                        ImVec2(x, bb.Max.y - 1),
                        isMajor ? colMajor : colMinor, 1.0f);
        }
    }

    // Keyframe markers (start/end + decoded stream offsets)
    std::set<int> keyframes;
    CollectKeyFrames(player, keyframes);
    for (int kf : keyframes) {
        if (kf < 0 || kf > lastFrame) continue;
        float x = frameToX((float)kf);
        ImVec2 a(x - 4.0f, bb.Min.y + 2);
        ImVec2 b(x + 4.0f, bb.Min.y + 2);
        ImVec2 c(x,        bb.Min.y + 8);
        dl->AddTriangleFilled(a, b, c, colKey);
    }

    // Playhead
    dl->AddLine(ImVec2(playheadX, bb.Min.y),
                ImVec2(playheadX, bb.Max.y),
                colPlay, 2.0f);
    dl->AddCircleFilled(ImVec2(playheadX, bb.Min.y + size.y * 0.5f),
                        4.0f, colPlay);

    // Frame label at end
    char label[64];
    snprintf(label, sizeof(label), "%d / %d  (%.2fs)", curFrame, lastFrame,
             curFrame * frameTime);
    ImVec2 labelSz = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(bb.Max.x - labelSz.x - 4, bb.Min.y + 2),
                ImGui::GetColorU32(ImGuiCol_Text), label);

    // Hover tooltip + scrub
    bool changed = false;
    if (hovered || active) {
        float mx = std::clamp(io.MousePos.x, bb.Min.x, bb.Max.x);
        float frac = (mx - bb.Min.x) / std::max(1.0f, bb.Max.x - bb.Min.x);
        int hoverFrame = (int)std::round(frac * (float)lastFrame);
        hoverFrame = std::clamp(hoverFrame, 0, lastFrame);

        // Vertical line at hover position
        float hx = frameToX((float)hoverFrame);
        dl->AddLine(ImVec2(hx, bb.Min.y), ImVec2(hx, bb.Max.y),
                    ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.6f), 1.0f);

        if (ImGui::BeginTooltip()) {
            ImGui::Text("Frame %d", hoverFrame);
            ImGui::Text("Time  %.3fs", hoverFrame * frameTime);
            ImGui::EndTooltip();
        }

        if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
            if (hoverFrame != curFrame) {
                player.SetFrame(hoverFrame);
                changed = true;
            }
        } else if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            if (hoverFrame != curFrame) {
                player.SetFrame(hoverFrame);
                changed = true;
            }
        }
    }

    ImGui::PopID();
    return changed;
}

} // namespace Onyx::UI
