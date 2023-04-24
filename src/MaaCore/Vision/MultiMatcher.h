#pragma once
#include "Matcher.h"
#include "Vision/Config/MatcherConfig.h"

MAA_NS_BEGIN

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

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }
};

MAA_NS_END
