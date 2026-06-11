#pragma once
#include <cstdint>
#include <functional>

namespace Onyx::Types {

// Opaque, runtime-assigned asset-type identity. value 0 == Unknown/invalid.
// Concrete types live in the app and are registered in the TypeCatalog
// (see GameTypes.h); the engine never enumerates them.
struct TypeId {
    uint32_t value = 0;
    constexpr bool operator==(const TypeId& o) const { return value == o.value; }
    constexpr bool operator!=(const TypeId& o) const { return value != o.value; }
    constexpr bool valid() const { return value != 0; }
};

} // namespace Onyx::Types

// Backwards-compat alias
namespace Onyx { using TypeId = Types::TypeId; }

template<> struct std::hash<Onyx::Types::TypeId> {
    size_t operator()(const Onyx::Types::TypeId& t) const noexcept {
        return std::hash<uint32_t>()(t.value);
    }
};
