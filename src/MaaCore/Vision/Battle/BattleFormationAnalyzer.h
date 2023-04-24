#pragma once

#include "Vision/VisionHelper.h"

MAA_VISION_NS_BEGIN

class BattleFormationAnalyzer : public VisionHelper
{
public:
    struct Result
    {
        std::string name;
        cv::Mat avatar;
    };
    using ResultsVec = std::vector<Result>;
    using ResultsVecOpt = std::optional<ResultsVec>;

public:
    using VisionHelper::VisionHelper;
    virtual ~BattleFormationAnalyzer() override = default;

    ResultsVecOpt analyze() const;
};
}

MAA_VISION_NS_END
