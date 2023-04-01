#pragma once

#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    class BattleFormationImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BattleFormationImageAnalyzer() override = default;

        virtual bool analyze() override;

        const auto& get_result() const { return m_result; }

    private:
        std::unordered_map<std::string, cv::Mat> m_result;
    };
}
