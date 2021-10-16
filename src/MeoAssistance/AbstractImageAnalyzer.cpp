#include "AbstractImageAnalyzer.h"

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat& image, const Rect& roi)
    : m_image(image),
    m_roi(empty_rect_to_full(roi, image))
#ifdef LOG_TRACE
    ,
    m_image_draw(image.clone())
#endif
{
    ;
}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat& image, const Rect& roi)
{
    m_image = image;
    m_roi = empty_rect_to_full(roi, image);
#ifdef LOG_TRACE
    m_image_draw = image.clone();
#endif
}

void asst::AbstractImageAnalyzer::set_roi(const Rect& roi) noexcept
{
    m_roi = empty_rect_to_full(roi, m_image);
}

asst::Rect asst::AbstractImageAnalyzer::empty_rect_to_full(const Rect& rect, const cv::Mat& image) noexcept
{
    return rect.empty() ? Rect(0, 0, image.cols, image.rows) : rect;
}