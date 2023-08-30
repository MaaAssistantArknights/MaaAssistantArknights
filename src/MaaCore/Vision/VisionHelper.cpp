#include "VisionHelper.h"

#include "Utils/NoWarningCV.h"

#include "Assistant.h"
#include "InstHelper.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Utils/Time.hpp"

using namespace asst;

VisionHelper::VisionHelper(const cv::Mat& image, const Rect& roi, Assistant* inst)
    : InstHelper(inst), m_image(image), m_roi(correct_rect(roi, image))
#ifdef ASST_DEBUG
      ,
      m_image_draw(image.clone())
#endif
{}

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

double asst::VisionHelper::get_image_entropy()
{
    // < 5ms
    cv::Mat grayed, hist;
    cv::cvtColor(m_image, grayed, cv::COLOR_BGR2GRAY);
    constexpr int channels[1] = { 0 };
    constexpr int histSize[1] = { 256 };
    constexpr float pranges[2] = { 0, 256 };
    const float* ranges[1] = { pranges };
    cv::calcHist(&grayed, 1, channels, cv::Mat(), hist, 1, histSize, ranges);
    const double total = grayed.size().area();
    hist /= total;
    double entropy = 0;
    const float* hist_data = hist.ptr<float>(0);
    for (int i = 0; i < 256; i++)
        if (hist_data[i] != 0)
            entropy += -cv::log(hist_data[i]) * hist_data[i];
    return entropy;
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
