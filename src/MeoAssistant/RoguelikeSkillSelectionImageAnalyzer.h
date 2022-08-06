#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class RoguelikeSkillSelectionImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        static constexpr size_t MaxNumOfSkills = 3;
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~RoguelikeSkillSelectionImageAnalyzer() override = default;

        virtual bool analyze() override;

        const auto& get_result() const noexcept
        {
            return m_result;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override
        {
            AbstractImageAnalyzer::set_roi(roi);
        }

        std::string name_analyze(const Rect& roi);
        std::vector<Rect> skill_analyze(const Rect& roi);

        std::unordered_map<std::string, std::vector<Rect>> m_result;    // 干员名 : 技能的位置（多个技能）
    };
}
