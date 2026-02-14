#include "VisionHelper.h"

#include "MaaUtils/NoWarningCV.hpp"

#include "Assistant.h"
#include "InstHelper.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Time.hpp"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

using namespace asst;

VisionHelper::VisionHelper(const cv::Mat& image, const Rect& roi, Assistant* inst) :
    InstHelper(inst),
    m_image(image),
#ifdef ASST_DEBUG
    m_image_draw(image.clone()),
#endif
    m_roi(correct_rect(roi, image))
{
}

void VisionHelper::set_image(const cv::Mat& image)
{
    m_image = image;
#ifdef ASST_DEBUG
    m_image_draw = image.clone();
#endif

    set_roi(m_roi);
}

void VisionHelper::set_roi(const Rect& roi)
{
    m_roi = correct_rect(roi, m_image);
}

void VisionHelper::set_log_tracing(bool enable)
{
    m_log_tracing = enable;
}

Rect asst::VisionHelper::correct_rect(const Rect& rect, const Rect& main_roi)
{
    if (main_roi.empty()) {
        return { 0, 0, 0, 0 };
    }

    Rect res = rect;
    if (res.x < 0) {
        res.x = 0;
        res.width = rect.width + rect.x;
    }
    if (res.y < 0) {
        res.y = 0;
        res.height = rect.height + rect.y;
    }
    if (res.x < main_roi.x) {
        res.x = main_roi.x;
        res.width = res.width - (main_roi.x - res.x);
    }
    if (res.y < main_roi.y) {
        res.y = main_roi.y;
        res.height = res.height - (main_roi.y - res.y);
    }
    if (res.x + res.width > main_roi.x + main_roi.width) {
        res.width = main_roi.x + main_roi.width - res.x;
    }
    if (res.y + res.height > main_roi.y + main_roi.height) {
        res.height = main_roi.y + main_roi.height - res.y;
    }
    return res;
}

Rect VisionHelper::correct_rect(const Rect& rect, const cv::Mat& image)
{
    if (image.empty() || image.cols <= 0 || image.rows <= 0) {
        Log.error(__FUNCTION__, "image is empty");
        return rect;
    }
    if (rect.empty()) {
        return { 0, 0, image.cols, image.rows };
    }
    if (rect.x >= image.cols || rect.y >= image.rows || rect.x + rect.width < 0 || rect.y + rect.height < 0) {
        Log.error(__FUNCTION__, "roi is out of range", image.cols, image.rows, rect.to_string());
        return { 0, 0, 0, 0 };
    }

    Rect res = rect;
    res.x = std::clamp(res.x, 0, image.cols - 1);
    res.y = std::clamp(res.y, 0, image.rows - 1);
    if (res.x > rect.x) {
        res.width = rect.width - (res.x - rect.x);
    }
    if (res.y > rect.y) {
        res.height = rect.height - (res.y - rect.y);
    }
    res.width = std::clamp(res.width, 1, image.cols - res.x);
    res.height = std::clamp(res.height, 1, image.rows - res.y);

    if (res != rect) {
        Log.warn(__FUNCTION__, "roi is out of range", image.cols, image.rows, rect.to_string(), "clamped");
    }

    return res;
}

cv::Mat asst::VisionHelper::create_mask(const cv::Mat& image, bool green_mask)
{
    cv::Mat mask = cv::Mat::ones(image.size(), CV_8UC1);
    if (green_mask) {
        cv::inRange(image, cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 0), mask);
        mask = ~mask;
    }
    return mask;
}

cv::Mat asst::VisionHelper::create_mask(const cv::Mat& image, const cv::Rect& roi)
{
    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
    mask(roi) = 255;
    return mask;
}

bool VisionHelper::save_img(const std::filesystem::path& relative_dir)
{
    bool ret = utils::save_debug_image(m_image, relative_dir, true);

#ifdef ASST_DEBUG
    utils::save_debug_image(m_image_draw, relative_dir, true, "", "draw");
#endif

    return ret;
}

cv::Mat VisionHelper::draw_roi(const cv::Rect& roi, const cv::Mat& base) const
{
    cv::Mat image_draw = base.empty() ? m_image.clone() : base;
    const cv::Scalar color(0, 255, 0);

    // cv::putText(image_draw, name_, cv::Point(5, m_image.rows - 5), cv::FONT_HERSHEY_SIMPLEX, 1, color, 2);

    cv::rectangle(image_draw, roi, color, 1);
    // std::string flag = MAA_FMT::format("ROI: [{}, {}, {}, {}]", roi.x, roi.y, roi.width, roi.height);
    std::string flag = "ROI: [" + std::to_string(roi.x) + ", " + std::to_string(roi.y) + ", " +
                       std::to_string(roi.width) + ", " + std::to_string(roi.height) + "]";
    cv::putText(image_draw, flag, cv::Point(roi.x, roi.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);

    return image_draw;
}
