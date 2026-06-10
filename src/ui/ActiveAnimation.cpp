#include "ui/ActiveAnimation.h"

namespace Onyx::UI {

static Onyx::AnimationPlayer* s_activePlayer = nullptr;

void SetActiveAnimationPlayer(Onyx::AnimationPlayer* p) { s_activePlayer = p; }
Onyx::AnimationPlayer* GetActiveAnimationPlayer()       { return s_activePlayer; }

} // namespace Onyx::UI
