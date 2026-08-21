#pragma once

// `GoWTool render` — draws an asset offscreen and writes a PNG, plus an
// optional JSON report of what the renderer actually got per batch.
//
// The point of the command is evidence: it drives the same SceneRenderer the
// viewport drives, so a PNG from here and a screenshot of the app disagree
// only when something real disagrees.

#include <string>
#include <vector>

namespace Onyx::Cli {

int RunRenderCommand(const std::vector<std::string>& args);
void PrintRenderHelp();

} // namespace Onyx::Cli
