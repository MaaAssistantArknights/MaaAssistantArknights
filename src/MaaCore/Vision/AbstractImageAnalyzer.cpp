#include "AbstractImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Assistant.h"
#include "InstHelper.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Utils/Time.hpp"

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat& image)
    : m_image(image), m_roi(correct_rect(Rect(), image))
#ifdef ASST_DEBUG
      ,
      m_image_draw(image.clone())
#endif
{}

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat& image, Assistant* inst)
    : InstHelper(inst), m_image(image), m_roi(correct_rect(Rect(), image))
#ifdef ASST_DEBUG
      ,
      m_image_draw(image.clone())
#endif
{}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat& image)
{
    m_image = image;
#ifdef ASST_DEBUG
    m_image_draw = image.clone();
#endif
}

void asst::AbstractImageAnalyzer::set_roi(const Rect& roi) noexcept
{
    m_roi = correct_rect(roi, m_image);
}

asst::Rect asst::AbstractImageAnalyzer::correct_rect(const Rect& rect, const cv::Mat& image) noexcept
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

bool asst::AbstractImageAnalyzer::save_img(const std::string& dirname)
{
    std::string stem = utils::get_format_time();
    stem = utils::string_replace_all(stem, { { ":", "-" }, { " ", "_" } });
    std::filesystem::create_directories(dirname);
    std::string full_path = dirname + stem + "_raw.png";
    Log.trace("Save image", full_path);
    bool ret = asst::imwrite(full_path, m_image);

#ifdef ASST_DEBUG
    asst::imwrite(dirname + stem + "_draw.png", m_image_draw);
#endif

    return ret;
}