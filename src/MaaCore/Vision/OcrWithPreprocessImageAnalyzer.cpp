#include "OcrWithPreprocessImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

const asst::OcrWithPreprocessImageAnalyzer::ResultsVecOpt& asst::OcrWithPreprocessImageAnalyzer::analyze()
{
    m_without_det = true;

    m_roi = correct_rect(m_roi, m_image);
    cv::Mat img_roi = m_image(make_rect<cv::Rect>(m_roi));
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
    OcrImageAnalyzer::set_roi(new_roi);
#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(new_roi), cv::Scalar(0, 0, 255), 1);
#endif // ASST_DEBUG

    return OcrImageAnalyzer::analyze();
}

void asst::OcrWithPreprocessImageAnalyzer::set_threshold(int lower, int upper)
{
    m_threshold_lower = lower;
    m_threshold_upper = upper;
}

void asst::OcrWithPreprocessImageAnalyzer::set_split(bool split)
{
    m_split = split;
}

void asst::OcrWithPreprocessImageAnalyzer::set_expansion(int expansion)
{
    m_expansion = expansion;
}
