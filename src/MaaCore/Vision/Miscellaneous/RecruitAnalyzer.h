#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstTypes.h"
#include "Vision/OCRer.h"

MAA_VISION_NS_BEGIN

class RecruitAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    RecruitAnalyzer(const cv::Mat& image, const Rect& roi) = delete;
    virtual ~RecruitAnalyzer() override = default;

    bool analyze();

    const std::vector<OCRer::Result>& get_tags_result() const noexcept { return m_tags_result; }
    Rect get_hour_decrement_rect() const noexcept { return m_hour_decrement; }
    Rect get_minute_decrement_rect() const noexcept { return m_minute_decrement; }
    Rect get_refresh_rect() const noexcept { return m_refresh_rect; }

private:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;
    bool tags_analyze();
    bool time_analyze();
    bool refresh_analyze();

    std::vector<OCRer::Result> m_tags_result;
    Rect m_hour_decrement;
    Rect m_minute_decrement;
    Rect m_refresh_rect;
};

MAA_VISION_NS_END
