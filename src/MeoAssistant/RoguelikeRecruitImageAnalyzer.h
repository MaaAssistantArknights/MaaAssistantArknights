#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class RoguelikeRecruitImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        struct RecruitOperInfo
        {
            std::string name;
            Rect rect;
            int elite = 0;
            int level = 0;
        };
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~RoguelikeRecruitImageAnalyzer() noexcept = default;

        bool analyze() override;

        const auto& get_result() const noexcept
        {
            return m_result;
        }
    private:

        int match_elite(const Rect& raw_roi);
        int match_level(const Rect& raw_roi);

        std::vector<RecruitOperInfo> m_result;
    };
}
