#pragma once

#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    class BattleSkillReadyImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BattleSkillReadyImageAnalyzer() override = default;

        virtual bool analyze() override;

        void set_base_point(const Point& pt);

    private:
        MatchRect m_result;
    };
}
