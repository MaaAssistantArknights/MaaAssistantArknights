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

    cv::Rect bounding_rect = cv::boundingRect(bin);
    bounding_rect.x += m_roi.x;
    bounding_rect.y += m_roi.y;
    auto new_roi = make_rect<Rect>(bounding_rect);

    if (new_roi.empty()) {
        return std::nullopt;
    }

    int exp = m_params.bin_expansion;
    if (exp) {
        new_roi.x -= exp;
        new_roi.y -= exp;
        new_roi.width += 2 * exp;
        new_roi.height += 2 * exp;
    }

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(new_roi), cv::Scalar(0, 0, 255), 1);
#endif // ASST_DEBUG

    OCRer ocr_analyzer(m_image, new_roi);
    auto config = m_params;
    config.without_det = true;
    ocr_analyzer.set_params(std::move(config));

    auto result = ocr_analyzer.analyze();
    if (!result) {
        return std::nullopt;
    }
    m_result = result->front();
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
