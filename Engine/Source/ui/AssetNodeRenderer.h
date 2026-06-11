#pragma once
#include "core/schema/AssetNode.h"

namespace Onyx {

// Render an entire AssetNode tree into the current ImGui context.
// Call from InfoTab — replaces hand-wired switch-case rendering.
void RenderAssetNode(const AssetNode& node);

} // namespace Onyx
