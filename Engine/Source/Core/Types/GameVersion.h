#pragma once
#include <cstdint>

namespace Onyx::Types {

/// Game version — used as part of the composite key for type handler dispatch.
///
/// GOW1 (PS2, 2005) is intentionally not yet supported. A partial stub used
/// to live here but produced half-wired handlers never exercised end-to-end.
/// When real GOW1 support lands, re-introduce the enum value alongside a
/// full ProfileGOW1 + handler/parser set rather than piggybacking on the
/// GOW2 code paths.
enum class GameVersion : uint8_t {
    GOW2,
    GOWR,  // God of War Ragnarök
};

} // namespace Onyx::Types

// Backwards-compat alias
namespace Onyx { using GameVersion = Types::GameVersion; }
