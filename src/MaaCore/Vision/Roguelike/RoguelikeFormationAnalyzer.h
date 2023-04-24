#pragma once
#include "Vision/VisionHelper.h"

MAA_VISION_NS_BEGIN

class RoguelikeFormationAnalyzer final : public VisionHelper
{
public:
    struct FormationOper
    {
        Rect rect;
        bool selected = false;
        // TODO
    };

public:
    using VisionHelper::VisionHelper;
    virtual ~RoguelikeFormationAnalyzer() override = default;

    bool analyze();

    const std::vector<FormationOper>& get_result() const noexcept { return m_result; }

protected:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;

    bool selected_analyze(const Rect& roi);

    std::vector<FormationOper> m_result;
};

MAA_VISION_NS_END
