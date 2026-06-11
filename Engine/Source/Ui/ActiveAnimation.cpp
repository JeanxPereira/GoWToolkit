#include "Ui/ActiveAnimation.h"

namespace Onyx::UI {

static Onyx::Rendering::AnimationPlayer* s_activePlayer = nullptr;

void SetActiveAnimationPlayer(Onyx::Rendering::AnimationPlayer* p) { s_activePlayer = p; }
Onyx::Rendering::AnimationPlayer* GetActiveAnimationPlayer()       { return s_activePlayer; }

} // namespace Onyx::UI
