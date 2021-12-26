#include "AbstractImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Controller.h"

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat image)
    : m_image(image), m_roi(empty_rect_to_full(Rect(), image))
#ifdef LOG_TRACE
    ,
    m_image_draw(image.clone())
#endif
{}

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat image, const Rect& roi)
    : m_image(image),
    m_roi(empty_rect_to_full(roi, image))
#ifdef LOG_TRACE
    ,
    m_image_draw(image.clone())
#endif
{
    ;
}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat image)
{
    m_image = image;
#ifdef LOG_TRACE
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

void asst::AbstractImageAnalyzer::correct_roi() noexcept
{
    m_roi = Ctrler.shaped_correct(m_roi);
}

asst::Rect asst::AbstractImageAnalyzer::empty_rect_to_full(const Rect& rect, const cv::Mat image) noexcept
{
    return rect.empty() ? Rect(0, 0, image.cols, image.rows) : rect;
}

std::string asst::AbstractImageAnalyzer::calc_name_hash() const
{
    return calc_name_hash(m_roi);
}

std::string asst::AbstractImageAnalyzer::calc_name_hash(const Rect& roi) const
{
    // 从左往右找到第一个白色点
    Rect white_roi = roi;
    constexpr static int HashKernelSize = 16;
    int threshold = 200;
    bool find_point = false;
    for (int i = 0; i != white_roi.width && !find_point; ++i) {
        for (int j = 0; j != white_roi.height && !find_point; ++j) {
            cv::Point point(white_roi.x + i, white_roi.y + j);
            auto value = m_image.at<cv::Vec3b>(point);
            if (value[0] > threshold && value[1] > threshold && value[2] > threshold) {
                white_roi.x += i;
                white_roi.width -= i;
                find_point = true;
                break;
            }
        }
    }
    cv::Mat image_roi = m_image(utils::make_rect<cv::Rect>(white_roi));
    cv::Mat bin;
    cv::cvtColor(image_roi, image_roi, cv::COLOR_BGR2GRAY);
    cv::threshold(image_roi, bin, threshold, 255, cv::THRESH_BINARY);
    cv::resize(bin, bin, cv::Size(HashKernelSize, HashKernelSize));
    std::stringstream hash_value;
    uchar* pix = bin.data;
    int tmp_dec = 0;
    for (int ro = 0; ro < 256; ro++) {
        tmp_dec = tmp_dec << 1;
        if ((bool)*pix)
            tmp_dec++;
        if (ro % 4 == 3) {
            hash_value << std::hex << tmp_dec;
            tmp_dec = 0;
        }
        pix++;
    }
    return hash_value.str();
}
