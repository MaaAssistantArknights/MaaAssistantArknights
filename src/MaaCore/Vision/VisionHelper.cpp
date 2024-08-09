#include "VisionHelper.h"

#include "Utils/NoWarningCV.h"

#include "Assistant.h"
#include "InstHelper.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Utils/Time.hpp"

using namespace asst;

VisionHelper::VisionHelper(const cv::Mat& image, const Rect& roi, Assistant* inst) :
    InstHelper(inst),
    m_image(image)
#ifdef ASST_DEBUG
    ,
    m_image_draw(image.clone())
#endif
    ,
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

Rect VisionHelper::correct_rect(const Rect& rect, const cv::Mat& image)
{
    if (image.empty()) {
        Log.error(__FUNCTION__, "image is empty");
        return rect;
    }
    if (rect.empty()) {
        return { 0, 0, image.cols, image.rows };
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

    if (res.x < 0) {
        Log.warn("roi is out of range", image.cols, image.rows, res.to_string());
        res.x = 0;
    }
    if (res.y < 0) {
        Log.warn("roi is out of range", image.cols, image.rows, res.to_string());
        res.y = 0;
    }
    if (image.cols < res.x + res.width) {
        Log.warn("roi is out of range", image.cols, image.rows, res.to_string());
        res.width = image.cols - res.x;
    }
    if (image.rows < res.y + res.height) {
        Log.warn("roi is out of range", image.cols, image.rows, res.to_string());
        res.height = image.rows - res.y;
    }
    return res;
}

bool VisionHelper::save_img(const std::filesystem::path& relative_dir)
{
    std::string stem = utils::get_time_filestem();
    auto relative_path = relative_dir / (stem + "_raw.png");
    Log.trace("Save image", relative_path);
    bool ret = imwrite(relative_path, m_image);

#ifdef ASST_DEBUG
    imwrite(relative_dir / (stem + "_draw.png"), m_image_draw);
#endif

    return ret;
}
