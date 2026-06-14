#pragma once
#include <Onyx/Schema/AssetNode.h>

namespace Onyx::App {

// Render an entire AssetNode tree into the current ImGui context.
// Call from InfoTab â€” replaces hand-wired switch-case rendering.
void RenderAssetNode(const Schema::AssetNode& node);

} // namespace Onyx::App
