#include "ui/ActiveAnimation.h"

namespace GOW::UI {

static GOW::AnimationPlayer* s_activePlayer = nullptr;

void SetActiveAnimationPlayer(GOW::AnimationPlayer* p) { s_activePlayer = p; }
GOW::AnimationPlayer* GetActiveAnimationPlayer()       { return s_activePlayer; }

} // namespace GOW::UI
