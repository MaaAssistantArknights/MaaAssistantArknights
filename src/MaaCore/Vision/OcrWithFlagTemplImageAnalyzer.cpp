#include "OcrWithFlagTemplImageAnalyzer.h"

#include "Config/TaskData.h"

asst::OcrWithFlagTemplImageAnalyzer::OcrWithFlagTemplImageAnalyzer(const cv::Mat& image, const Rect& roi)
    : OcrWithPreprocessImageAnalyzer(image, roi), m_multi_match_image_analyzer(image)
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

const asst::OcrWithFlagTemplImageAnalyzer::ResultsVecOpt& asst::OcrWithFlagTemplImageAnalyzer::analyze()
{
    m_result = std::nullopt;

    auto matched_vec_opt = m_multi_match_image_analyzer.analyze();
    if (!matched_vec_opt) {
        return std::nullopt;
    }
    auto& matched_vec = *matched_vec_opt;

    ResultsVec results;
    for (const auto& matched : matched_vec) {
        Rect roi = matched.rect.move(m_flag_rect_move);
        set_roi(roi);

        auto ocr_opt = OcrWithPreprocessImageAnalyzer::analyze();
        if (!ocr_opt) {
            continue;
        }
        auto& ocr_res = *ocr_opt;
        results.insert(results.end(), std::make_move_iterator(ocr_res.begin()), std::make_move_iterator(ocr_res.end()));
    }

    if (results.empty()) {
        return std::nullopt;
    }
    m_result = results;

    return m_result;
}

void asst::OcrWithFlagTemplImageAnalyzer::set_task_info(const std::string& templ_task_name,
                                                        const std::string& ocr_task_name)
{
    auto ocr_task_ptr = Task.get<OcrTaskInfo>(ocr_task_name);
    _set_task_info(*ocr_task_ptr);
    m_flag_rect_move = ocr_task_ptr->roi;
    m_multi_match_image_analyzer.set_task_info(templ_task_name);
}

void asst::OcrWithFlagTemplImageAnalyzer::set_flag_rect_move(Rect flag_rect_move)
{
    m_flag_rect_move = flag_rect_move;
}
