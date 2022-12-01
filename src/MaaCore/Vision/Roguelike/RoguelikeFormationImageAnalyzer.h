#pragma once
#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    class RoguelikeFormationImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        struct FormationOper
        {
            Rect rect;
            bool selected = false;
            // TODO
        };

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~RoguelikeFormationImageAnalyzer() override = default;

        virtual bool analyze() override;

        const std::vector<FormationOper>& get_result() const noexcept;

    protected:
        // 该分析器不支持外部设置ROI
        using AbstractImageAnalyzer::set_roi;

        bool selected_analyze(const Rect& roi);

        std::vector<FormationOper> m_result;
    };
}
