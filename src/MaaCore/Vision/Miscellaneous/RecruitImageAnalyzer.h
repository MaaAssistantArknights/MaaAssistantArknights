#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstTypes.h"

namespace asst
{
class RecruitImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    RecruitImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;
    virtual ~RecruitImageAnalyzer() override = default;

    bool analyze();

    const std::vector<TextRect>& get_tags_result() const noexcept { return m_tags_result; }

    Rect get_hour_decrement_rect() const noexcept { return m_hour_decrement; }

    Rect get_minute_decrement_rect() const noexcept { return m_minute_decrement; }

    Rect get_refresh_rect() const noexcept { return m_refresh_rect; }

    Rect get_permit_rect() const noexcept { return m_permit_rect; }

#ifdef ASST_DEBUG
    // mock function
    enum special_type
    {
        top_operator = 6,
        senior_operator = 5
    };
   
    void mock_set_special(special_type type)
    {
        if (type == top_operator) {
            m_tags_result[0].text = "高级资深干员";
        }
        if (type == senior_operator) {
            m_tags_result[0].text = "资深干员";
        }
    }
#endif                                                          // ASST_DEBUG

private:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;
    bool tags_analyze();
    bool time_analyze();
    bool refresh_analyze();
    bool permit_analyze();

    std::vector<TextRect> m_tags_result;
    Rect m_hour_decrement;
    Rect m_minute_decrement;
    Rect m_refresh_rect;
    Rect m_permit_rect;
};
}
