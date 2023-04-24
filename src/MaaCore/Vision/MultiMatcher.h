#pragma once
#include "Matcher.h"
#include "Vision/Config/MatcherConfig.h"

MAA_VISION_NS_BEGIN

class MultiMatcher : public VisionHelper, public MatcherConfig
{
public:
    using Result = Matcher::Result;
    using ResultsVec = std::vector<Result>;
    using ResultsVecOpt = std::optional<ResultsVec>;

public:
    using VisionHelper::VisionHelper;
    virtual ~MultiMatcher() override = default;

    ResultsVecOpt analyze() const;
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    ResultsVec get_result() const { return m_result; }

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

private:
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    mutable ResultsVec m_result;
};

MAA_VISION_NS_END
