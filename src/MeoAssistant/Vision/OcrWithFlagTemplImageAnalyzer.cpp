#include "OcrWithFlagTemplImageAnalyzer.h"

#include "Config/TaskData.h"

asst::OcrWithFlagTemplImageAnalyzer::OcrWithFlagTemplImageAnalyzer(const cv::Mat& image)
    : OcrWithPreprocessImageAnalyzer(image), m_multi_match_image_analyzer(image)
{}

void asst::OcrWithFlagTemplImageAnalyzer::set_image(const cv::Mat& image)
{
    OcrWithPreprocessImageAnalyzer::set_image(image);
    m_multi_match_image_analyzer.set_image(image);
}

void asst::OcrWithFlagTemplImageAnalyzer::set_roi(const Rect& roi) noexcept
{
    OcrWithPreprocessImageAnalyzer::set_roi(roi);
    m_multi_match_image_analyzer.set_roi(roi);
}

bool asst::OcrWithFlagTemplImageAnalyzer::analyze()
{
    m_all_result.clear();

    if (!m_multi_match_image_analyzer.analyze()) {
        return false;
    }
    for (const auto& templ_res : m_multi_match_image_analyzer.get_result()) {
        Rect roi = templ_res.rect.move(m_flag_rect_move);
        if (roi.x + roi.width >= WindowWidthDefault) {
            continue;
        }
        set_roi(roi);

        if (OcrWithPreprocessImageAnalyzer::analyze()) {
            m_all_result.insert(m_all_result.end(), std::make_move_iterator(m_ocr_result.begin()),
                                std::make_move_iterator(m_ocr_result.end()));
        }
    }

    return !m_all_result.empty();
}

const std::vector<asst::TextRect>& asst::OcrWithFlagTemplImageAnalyzer::get_result() const noexcept
{
    return m_all_result;
}

void asst::OcrWithFlagTemplImageAnalyzer::set_task_info(const std::string& templ_task_name,
                                                        const std::string& ocr_task_name)
{
    auto ocr_task_ptr = Task.get<OcrTaskInfo>(ocr_task_name);
    set_task_info(*ocr_task_ptr);
    m_flag_rect_move = ocr_task_ptr->roi;
    m_multi_match_image_analyzer.set_task_info(templ_task_name);
}

void asst::OcrWithFlagTemplImageAnalyzer::set_flag_rect_move(Rect flag_rect_move)
{
    m_flag_rect_move = flag_rect_move;
}

std::vector<asst::TextRect>& asst::OcrWithFlagTemplImageAnalyzer::get_result() noexcept
{
    return m_all_result;
}
