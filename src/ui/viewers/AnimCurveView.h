#pragma once
#include "ui/IPanel.h"
#include "core/parsers/shared/AnimationData.h"
#include <vector>
#include <string>
#include <memory>

/// Panel that visualizes animation curves (joint rotation/position over time)
/// using ImPlot. Displays curves from whatever AnimationData is currently available
/// via the selected asset's SceneData or via the global animation pointer.
class AnimCurveView : public IPanel {
public:
    AnimCurveView();
    ~AnimCurveView();

    void draw(AppContext& ctx) override;
    std::string_view getName() const override { return "Anim Curves"; }

private:
    std::shared_ptr<GOW::AnimationData> m_animData;
    int m_selectedGroup = 0;
    int m_selectedAct   = 0;
    int m_selectedState = 0;
    int m_selectedJoint = 0;
    bool m_showRotation = true;
    bool m_showPosition = true;
};
