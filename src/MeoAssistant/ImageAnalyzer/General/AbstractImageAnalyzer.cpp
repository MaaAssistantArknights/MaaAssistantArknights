#include "AbstractImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Controller.h"
#include "Utils/AsstImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Utils/Time.hpp"
#include "Utils/UserDir.hpp"

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat& image)
    : m_image(image), m_roi(empty_rect_to_full(Rect(), image))
#ifdef ASST_DEBUG
      ,
      m_image_draw(image.clone())
#endif
{}

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat& image, const Rect& roi)
    : m_image(image), m_roi(empty_rect_to_full(roi, image))
#ifdef ASST_DEBUG
      ,
      m_image_draw(image.clone())
#endif
{}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat image)
{
    m_image = image;
#ifdef ASST_DEBUG
    m_image_draw = image.clone();
#endif
}

void asst::AbstractImageAnalyzer::set_roi(const Rect& roi) noexcept
{
    m_roi = empty_rect_to_full(roi, m_image);
}

asst::Rect asst::AbstractImageAnalyzer::empty_rect_to_full(const Rect& rect, const cv::Mat& image) noexcept
{
    if (image.empty()) {
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

bool asst::AbstractImageAnalyzer::save_img(const std::string& dirname)
{
    auto user_dir = asst::UserDir::get_instance().get();
    auto outdir = (user_dir / dirname).string();

    std::string stem = utils::get_format_time();
    stem = utils::string_replace_all(stem, { { ":", "-" }, { " ", "_" } });
    std::filesystem::create_directories(outdir);
    std::string full_path = outdir + stem + "_raw.png";
    Log.trace("Save image", full_path);
    bool ret = asst::imwrite(full_path, m_image);

#ifdef ASST_DEBUG
    asst::imwrite(outdir + stem + "_draw.png", m_image_draw);
#endif

    return ret;
}
