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
        const std::vector<MatchRect>& get_result() const noexcept { return m_result; }

        void set_base_point(const Point& pt);

    private:
        std::vector<MatchRect> m_result;
    };
}
