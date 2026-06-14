#pragma once
#include "imgui.h"
#include <Onyx/Rendering/AnimationPlayer.h>

namespace Onyx::App {

// Rich timeline widget for skeletal animations.
//
// Draws a custom track bar with frame ticks, keyframe markers (one per stream
// sample offset), a draggable playhead, and a hover tooltip. Returns true if
// the user moved the playhead (caller should mark the viewport dirty).
//
// The widget reads the player's current frame and writes back through
// SetFrame, so it works equally well while playing or paused.
struct AnimationTimelineStyle {
    float trackHeight    = 22.0f;
    float minorTickEvery = 1.0f;   // frames
    float majorTickEvery = 10.0f;  // frames
};

bool DrawAnimationTimeline(const char* strId,
                           Onyx::Rendering::AnimationPlayer& player,
                           const AnimationTimelineStyle& style = {});

} // namespace Onyx::App
