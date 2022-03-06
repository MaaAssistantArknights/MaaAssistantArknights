#include "AbstractImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Controller.h"

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat image)
    : m_image(image), m_roi(empty_rect_to_full(Rect(), image))
#ifdef ASST_DEBUG
    ,
    m_image_draw(image.clone())
#endif
{}

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat image, std::shared_ptr<TaskData> task_data)
    : m_image(image), m_roi(empty_rect_to_full(Rect(), image)),
    m_task_data(task_data)
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

void asst::AbstractImageAnalyzer::_set_image(const cv::Mat image)
{
    m_image = image;
#ifdef ASST_DEBUG
    m_image_draw = image.clone();
#endif
}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat image, const Rect& roi)
{
    _set_image(image);
    m_roi = empty_rect_to_full(roi, image);
}

void asst::AbstractImageAnalyzer::set_roi(const Rect& roi) noexcept
{
    m_roi = empty_rect_to_full(roi, m_image);
}

void asst::AbstractImageAnalyzer::set_task_data(std::shared_ptr<TaskData> task_data)
{
    m_task_data = task_data;
}

asst::Rect asst::AbstractImageAnalyzer::empty_rect_to_full(const Rect& rect, const cv::Mat image) noexcept
{
    return rect.empty() ? Rect(0, 0, image.cols, image.rows) : rect;
}
