#pragma once
#include <any>
#include <utility>

namespace GOW {

// Type-safe wrapper for opaque profile-specific tag data attached to ParsedEntry.
class ProfileTag {
    std::any m_data;

public:
    ProfileTag() = default;

    template<typename T>
    const T* As() const {
        return std::any_cast<T>(&m_data);
    }

    template<typename T>
    static ProfileTag Of(T v) {
        ProfileTag t;
        t.m_data = std::move(v);
        return t;
    }
};

} // namespace GOW
