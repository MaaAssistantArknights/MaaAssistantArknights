#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
    class BattleFormationImageAnalyzer : public VisionHelper
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
        virtual ~BattleFormationImageAnalyzer() override = default;

        const ResultsVecOpt& analyze();
        const ResultsVecOpt& result() const noexcept { return m_result; }
    };
}
