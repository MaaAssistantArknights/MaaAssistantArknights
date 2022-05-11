#include "AbstractImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Controller.h"
#include "Logger.hpp"

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat image)
    : m_image(image), m_roi(empty_rect_to_full(Rect(), image))
#ifdef ASST_DEBUG
    ,
    m_image_draw(image.clone())
#endif
{}

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat image, const Rect& roi)
    : m_image(image),
    m_roi(empty_rect_to_full(roi, image))
#ifdef ASST_DEBUG
    ,
    m_image_draw(image.clone())
#endif
{
    ;
}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat image)
{
    m_image = image;
#ifdef ASST_DEBUG
    m_image_draw = image.clone();
#endif
}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat image, const Rect& roi)
{
    set_image(image);
    m_roi = empty_rect_to_full(roi, image);
}

void asst::AbstractImageAnalyzer::set_roi(const Rect& roi) noexcept
{
    m_roi = empty_rect_to_full(roi, m_image);
}

asst::Rect asst::AbstractImageAnalyzer::empty_rect_to_full(const Rect& rect, const cv::Mat image) noexcept
{
    if (image.empty()) {
        return rect;
    }
    if (rect.empty()) {
        return Rect(0, 0, image.cols, image.rows);
    }

    Rect res = rect;
    if (image.cols < res.x) {
        Log.error("roi is out of range", image.cols, image.rows, res.to_string());
        res.x = image.cols - res.width;
    }
    if (image.rows < res.y) {
        Log.error("roi is out of range", image.cols, image.rows, res.to_string());
        res.y = image.rows - res.height;
    }

    if (image.cols < res.x + res.width) {
        Log.error("roi is out of range", res.to_string());
        res.width = image.cols - res.x;
    }
    if (image.rows < res.y + res.height) {
        Log.error("roi is out of range", res.to_string());
        res.height = image.rows - res.y;
    }
    return res;
}
