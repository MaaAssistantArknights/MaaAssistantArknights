#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
class RoguelikeFormationImageAnalyzer final : public VisionHelper
{
public:
    struct FormationOper
    {
        Rect rect;
        bool selected = false;
        std::string name;
        int page = 0;
        // TODO
    };

public:
    using VisionHelper::VisionHelper;
    virtual ~RoguelikeFormationImageAnalyzer() override = default;

    bool analyze();

    const std::vector<FormationOper>& get_result() const noexcept;

protected:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;

    bool selected_analyze(const Rect& roi);

    std::vector<FormationOper> m_result;
};
}
