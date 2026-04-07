#pragma once
#include <cstdint>

namespace GOW {

/// Game version — used as part of the composite key for type handler dispatch.
enum class GameVersion : uint8_t {
    GOW1,
    GOW2,
    GOWR,  // God of War Ragnarök
};

} // namespace GOW
