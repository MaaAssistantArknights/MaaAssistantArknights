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

#ifdef ASST_DEBUG
asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat& image, cv::Mat& draw)
    : m_image(image), m_roi(correct_rect(Rect(), image)), m_image_draw(draw)
{}
#endif

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

#ifdef ASST_DEBUG
void asst::AbstractImageAnalyzer::set_image(const cv::Mat& image, cv::Mat& draw)
{
    m_image = image;
    m_image_draw = draw;
}
#endif

void asst::AbstractImageAnalyzer::set_roi(const Rect& roi) noexcept
{
    m_roi = correct_rect(roi, m_image);
}

void asst::AbstractImageAnalyzer::set_log_tracing(bool enable)
{
    m_log_tracing = enable;
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

bool asst::AbstractImageAnalyzer::save_img(const std::filesystem::path& relative_dir)
{
    std::string stem = utils::get_time_filestem();
    auto relative_path = relative_dir / (stem + "_raw.png");
    Log.trace("Save image", relative_path);
    bool ret = asst::imwrite(relative_path, m_image);

#ifdef ASST_DEBUG
    asst::imwrite(relative_dir / (stem + "_draw.png"), m_image_draw);
#endif

    return ret;
}

#ifdef ASST_DEBUG
cv::Mat asst::AbstractImageAnalyzer::get_draw() const
{
    return m_image_draw;
}
#endif
