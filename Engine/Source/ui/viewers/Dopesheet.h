#pragma once
#include "ui/IPanel.h"
#include "core/parsers/shared/AnimationData.h"
#include <memory>
#include <string>

// Dopesheet panel — joints down, frames across.
//
// Each row is one joint, each cell a frame. A dot is drawn where the
// underlying decoder actually has a sample for that joint at that frame
// (sourced from the parsed AnimSubstream maps). The current playhead is
// drawn as a vertical highlight so users can correlate the viewport pose
// to the channel grid.
class Dopesheet : public IPanel {
public:
    Dopesheet();
    ~Dopesheet();

    void Draw() override;
    std::string_view getName() const override { return "Dopesheet"; }

private:
    std::shared_ptr<Onyx::AnimationData> m_animData;
    int  m_selectedGroup = 0;
    int  m_selectedAct   = 0;
    int  m_selectedState = 0;
    bool m_showRotation = true;
    bool m_showPosition = true;
    float m_zoom = 8.0f;  // pixels per frame
    float m_rowHeight = 14.0f;
    float m_jointLabelWidth = 80.0f;
};
