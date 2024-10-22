#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
class BrightPointAnalyzer : public VisionHelper
{
public:
    using Result = Point;
    using ResultsVec = std::vector<Result>;
    using ResultsVecOpt = std::optional<ResultsVec>;

    using VisionHelper::VisionHelper;
    virtual ~BrightPointAnalyzer() override = default;

    ResultsVecOpt analyze();

    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    const auto& get_result() const noexcept { return m_result; }

private:
    ResultsVec m_result;
};
}
