#pragma once

// Lightweight broker so cross-cutting UI panels (Anim Curves, Dopesheet)
// can read playhead state from whichever Viewport3D currently owns the
// active SceneRenderer + AnimationPlayer without taking on a hard
// dependency on the viewport itself.
//
// The viewport that's currently focused sets the pointer each frame;
// readers should treat it as a weak observer and tolerate nullptr.

namespace GOW {
class AnimationPlayer;
}

namespace GOW::UI {

void              SetActiveAnimationPlayer(GOW::AnimationPlayer* player);
GOW::AnimationPlayer* GetActiveAnimationPlayer();

} // namespace GOW::UI
