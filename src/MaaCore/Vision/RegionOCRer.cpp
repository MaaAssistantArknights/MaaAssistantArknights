#include "RegionOCRer.h"

#include "Utils/NoWarningCV.h"

using namespace asst;

RegionOCRer::ResultOpt RegionOCRer::analyze() const
{
    cv::Mat img_roi = make_roi(m_image, m_roi);
    cv::Mat img_roi_gray;
    cv::cvtColor(img_roi, img_roi_gray, cv::COLOR_BGR2GRAY);
    cv::Mat bin;
    cv::inRange(img_roi_gray, m_params.bin_threshold_lower, m_params.bin_threshold_upper, bin);

    bin_left_trim(bin);
    bin_right_trim(bin);

    auto bounding_rect = make_rect<Rect>(cv::boundingRect(bin));

    if (bounding_rect.empty()) {
        return std::nullopt;
    }
    auto expand_roi = [](Rect& roi, int exp) {
        if (exp == 0) return;
        roi.x -= exp;
        roi.y -= exp;
        roi.width += 2 * exp;
        roi.height += 2 * exp;
    };
    expand_roi(bounding_rect, m_params.bin_expansion);

    auto new_roi = bounding_rect;
    new_roi.x += m_roi.x;
    new_roi.y += m_roi.y;

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(new_roi), cv::Scalar(0, 0, 255), 1);
#endif // ASST_DEBUG

    OCRer ocr_analyzer;
    if (m_use_raw) {
        ocr_analyzer = OCRer(m_image, new_roi);
    }
    else {
        cv::Mat bin3;
        std::array arr_bin3 { bin, bin, bin };
        cv::merge(arr_bin3, bin3);
        ocr_analyzer = OCRer(bin3, bounding_rect);
    }

    auto config = m_params;
    config.without_det = true;
    ocr_analyzer.set_params(std::move(config));

    auto result = ocr_analyzer.analyze();
    if (!result) {
        return std::nullopt;
    }
    m_result = result->front();
    if (!m_use_raw) {
        m_result.rect.x += m_roi.x;
        m_result.rect.y += m_roi.y;
    }
    return m_result;
}

void asst::RegionOCRer::bin_left_trim(cv::Mat& bin) const
{
    if (!m_params.bin_left_trim_threshold) {
        return;
    }

    int pre_white_col = 0;
    for (int i = 0; i < bin.cols; ++i) {
        bool has_white = false;
        for (int j = 0; j < bin.rows; ++j) {
            if (bin.at<uchar>(j, i)) {
                has_white = true;
                break;
            }
        }
        if (has_white) {
            pre_white_col = i;
        }
        else if (i - pre_white_col > m_params.bin_left_trim_threshold) {
            bin.colRange(0, i).setTo(0);
            break;
        }
    }
}

void asst::RegionOCRer::bin_right_trim(cv::Mat& bin) const
{
    if (!m_params.bin_right_trim_threshold) {
        return;
    }

    int pre_white_col = bin.cols - 1;
    for (int i = bin.cols - 1; i >= 0; --i) {
        bool has_white = false;
        for (int j = 0; j < bin.rows; ++j) {
            if (bin.at<uchar>(j, i)) {
                has_white = true;
                break;
            }
        }
        if (has_white) {
            pre_white_col = i;
        }
        else if (pre_white_col - i > m_params.bin_right_trim_threshold) {
            bin.colRange(i, bin.cols).setTo(0);
            break;
        }
    }
}
