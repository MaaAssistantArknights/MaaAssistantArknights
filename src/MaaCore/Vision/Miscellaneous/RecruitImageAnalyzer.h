#pragma once
#include "Vision/AbstractImageAnalyzer.h"

#include "Common/AsstTypes.h"

namespace asst
{
    class RecruitImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        RecruitImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;
        virtual ~RecruitImageAnalyzer() override = default;

        virtual bool analyze() override;

        const std::vector<TextRect>& get_tags_result() const noexcept { return m_tags_result; }
        Rect get_hour_decrement_rect() const noexcept { return m_hour_decrement; }
        Rect get_minute_decrement_rect() const noexcept { return m_minute_decrement; }
        Rect get_refresh_rect() const noexcept { return m_refresh_rect; }

    private:
        // 该分析器不支持外部设置ROI
        using AbstractImageAnalyzer::set_roi;
        bool tags_analyze();
        bool time_analyze();
        bool refresh_analyze();

        std::vector<TextRect> m_tags_result;
        Rect m_hour_decrement;
        Rect m_minute_decrement;
        Rect m_refresh_rect;
    };
}
