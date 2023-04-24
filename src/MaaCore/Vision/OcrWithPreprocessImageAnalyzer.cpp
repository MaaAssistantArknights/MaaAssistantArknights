#include "OcrWithPreprocessImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

const asst::OcrWithPreprocessImageAnalyzer::ResultOpt& asst::OcrWithPreprocessImageAnalyzer::analyze()
{
    m_result = std::nullopt;
    m_params.without_det = true;

    cv::Mat img_roi = make_roi(m_image, m_roi);
    cv::Mat img_roi_gray;
    cv::cvtColor(img_roi, img_roi_gray, cv::COLOR_BGR2GRAY);
    cv::Mat bin;
    cv::inRange(img_roi_gray, m_threshold_lower, m_threshold_upper, bin);
    cv::Rect bounding_rect = cv::boundingRect(bin);
    bounding_rect.x += m_roi.x;
    bounding_rect.y += m_roi.y;
    auto new_roi = make_rect<Rect>(bounding_rect);

    if (new_roi.empty()) {
        return std::nullopt;
    }
    // todo: split

    if (m_expansion) {
        new_roi.x -= m_expansion;
        new_roi.y -= m_expansion;
        new_roi.width += 2 * m_expansion;
        new_roi.height += 2 * m_expansion;
    }

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(new_roi), cv::Scalar(0, 0, 255), 1);
#endif // ASST_DEBUG

    
    OcrImageAnalyzer ocr_analyzer(m_image, new_roi, m_inst);
    ocr_analyzer.set_params(m_params);
    if (!ocr_analyzer.analyze()) {
        return std::nullopt;
    }
    ocr_analyzer.sort_results_by_score();
    return ocr_analyzer.result()->front();
}

void asst::OcrWithPreprocessImageAnalyzer::set_threshold(int lower, int upper)
{
    m_threshold_lower = lower;
    m_threshold_upper = upper;
}

void asst::OcrWithPreprocessImageAnalyzer::set_expansion(int expansion)
{
    m_expansion = expansion;
}

// void asst::OcrWithPreprocessImageAnalyzer::set_split(bool split)
//{
//     m_split = split;
// }
