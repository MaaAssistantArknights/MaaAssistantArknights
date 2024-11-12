#pragma once
#include "Vision/Config/MatcherConfig.h"
#include "VisionHelper.h"

namespace asst
{
class MultiMatcher : public VisionHelper, public MatcherConfig
{
public:
    using Result = MatchRect;
    using ResultsVec = std::vector<Result>;
    using ResultsVecOpt = std::optional<ResultsVec>;

public:
    using VisionHelper::VisionHelper;
    virtual ~MultiMatcher() override = default;

    ResultsVecOpt analyze() const;

    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    const auto& get_result() const noexcept { return m_result; }

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

private:
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    mutable ResultsVec m_result;
};
}
