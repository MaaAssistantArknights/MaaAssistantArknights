#pragma once
#include "AbstractImageAnalyzer.h"

#include "AsstDef.h"

namespace asst
{
    class RecruitImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        RecruitImageAnalyzer(const cv::Mat image, const Rect& roi) = delete;
        virtual ~RecruitImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<TextRect>& get_tags_result() const noexcept
        {
            return m_tags_result;
        }
        const std::vector<Rect>& get_set_time_rect() const noexcept
        {
            return m_set_time_rect;
        }
        Rect get_confirm_rect() const noexcept
        {
            return m_confirm_rect;
        }
        Rect get_refresh_rect() const noexcept
        {
            return m_refresh_rect;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override
        {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat image, const Rect& roi)
        {
            AbstractImageAnalyzer::set_image(image, roi);
        }
        bool tags_analyze();
        bool time_analyze();
        bool confirm_analyze();
        bool refresh_analyze();

        std::vector<TextRect> m_tags_result;
        std::vector<Rect> m_set_time_rect;
        Rect m_confirm_rect;
        Rect m_refresh_rect;
    };
}
