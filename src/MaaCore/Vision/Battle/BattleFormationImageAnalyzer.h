#pragma once

#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    class BattleFormationImageAnalyzer : public AbstractImageAnalyzer
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
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BattleFormationImageAnalyzer() override = default;

        const ResultsVecOpt& analyze();
        const ResultsVecOpt& result() const noexcept { return m_result; }

    private:
        ResultsVecOpt m_result;
    };
}
