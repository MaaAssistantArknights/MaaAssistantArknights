#pragma once
#include "AbstractImageAnalyzer.h"

#include "AsstDef.h"

namespace asst {
    class RecruitImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~RecruitImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<TextRect>& get_tags_result() const noexcept {
            return m_tags_result;
        }
        const std::vector<Rect>& get_set_time_rect() const noexcept {
            return m_set_time_rect;
        }
    protected:
        bool tags_analyze();
        bool time_analyze();

        std::vector<TextRect> m_tags_result;
        std::vector<Rect> m_set_time_rect;
    };
}
