#pragma once
#include <string_view>

struct AppContext;

class IPanel {
public:
    virtual ~IPanel() = default;
    virtual void Draw() = 0;
    virtual std::string_view getName() const = 0;
    bool visible = true;
};
