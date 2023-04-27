#include "OcrWithPreprocessImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

using namespace asst;

OcrWithPreprocessImageAnalyzer::ResultOpt OcrWithPreprocessImageAnalyzer::analyze() const
{
    cv::Mat img_roi = make_roi(m_image, m_roi);
    cv::Mat img_roi_gray;
    cv::cvtColor(img_roi, img_roi_gray, cv::COLOR_BGR2GRAY);
    cv::Mat bin;
    cv::inRange(img_roi_gray, m_params.bin_threshold_lower, m_params.bin_threshold_upper, bin);
    cv::Rect bounding_rect = cv::boundingRect(bin);
    bounding_rect.x += m_roi.x;
    bounding_rect.y += m_roi.y;
    auto new_roi = make_rect<Rect>(bounding_rect);

    if (new_roi.empty()) {
        return std::nullopt;
    }
    // todo: split

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

    OcrImageAnalyzer ocr_analyzer(m_image, new_roi);
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
