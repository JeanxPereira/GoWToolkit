#pragma once
#include "Ui/IPanel.h"

namespace Onyx::App {

class CameraPanel : public IPanel {
public:
    CameraPanel();
    ~CameraPanel() override;

    void Draw() override;
    std::string_view getName() const override { return "Camera"; }

    // Singleton handle so the Viewport3D toolbar's "Cam" button can flip the
    // panel's visibility directly. View menu also drives IPanel::visible,
    // keeping both sources of truth in sync.
    static CameraPanel* Get() { return s_instance; }
    static void         Toggle();

private:
    static CameraPanel* s_instance;
};

} // namespace Onyx::App
