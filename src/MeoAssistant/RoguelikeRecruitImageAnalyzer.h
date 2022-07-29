#pragma once
#include "AbstractImageAnalyzer.h"

#include "AsstBattleDef.h"

namespace asst
{
    class RoguelikeRecruitImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~RoguelikeRecruitImageAnalyzer() noexcept override = default;

        bool analyze() override;

        const auto& get_result() const noexcept
        {
            return m_result;
        }
    private:

        int match_elite(const Rect& raw_roi);
        int match_level(const Rect& raw_roi);

        std::vector<BattleRecruitOperInfo> m_result;
    };
}
