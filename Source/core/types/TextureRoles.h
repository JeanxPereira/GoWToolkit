#pragma once

// Bridging between this app's texture-role vocabulary and Onyx v1.1's.
//
// Two role enums exist and neither can simply replace the other:
//
//   Onyx::Parsers::TextureRole  -- what a MaterialDesc can bind. Nine roles,
//       the set the renderer's samplers understand.
//   Onyx::TextureRole          -- what a Ragnarök material *declares*, parsed
//       from the `_0d_` / `_0n_` / `_0ao_` suffixes in texture names. Twelve
//       roles, because GOWR names things the renderer has no sampler for
//       (Specular, Roughness, Metallic) and spells one of them differently
//       (AmbientOcclusion vs Occlusion).
//
// Mapping is therefore partial by construction, and a role with no Onyx
// counterpart is not an error -- it is a texture this renderer cannot use.

#include <Onyx/Parsers/SceneNode.h>
#include "core/parsers/gowr/MaterialParser.h"

#include <optional>

namespace Onyx {

/// Human-readable name for a role a MaterialDesc can bind.
const char* SceneRoleName(Onyx::Parsers::TextureRole role);

/// The Onyx role a GOWR role maps onto, or nullopt when the renderer has no
/// sampler for it (Specular, Roughness, Metallic, Unknown).
std::optional<Onyx::Parsers::TextureRole> ToSceneRole(Onyx::TextureRole role);

} // namespace Onyx
