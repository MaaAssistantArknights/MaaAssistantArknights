#include "OcrWithFlagTemplImageAnalyzer.h"

#include "MultiMatchImageAnalyzer.h"
#include "OcrWithPreprocessImageAnalyzer.h"
#include "Config/TaskData.h"

const asst::OcrWithFlagTemplImageAnalyzer::ResultsVecOpt& asst::OcrWithFlagTemplImageAnalyzer::analyze()
{
    m_result = std::nullopt;

    MultiMatchImageAnalyzer flag_analyzer(m_image, m_roi, m_inst);
    flag_analyzer.set_params(MatchImageAnalyzerConfig::m_params);

    auto matched_vec_opt = flag_analyzer.analyze();
    if (!matched_vec_opt) {
        return std::nullopt;
    }
    auto& matched_vec = *matched_vec_opt;

    ResultsVec results;
    for (const auto& matched : matched_vec) {
        Rect roi = matched.rect.move(m_flag_rect_move);

        OcrWithPreprocessImageAnalyzer ocr_analyzer(m_image, roi, m_inst);
        ocr_analyzer.set_params(OcrImageAnalyzerConfig::m_params);
        auto ocr_opt = ocr_analyzer.analyze();
        if (!ocr_opt) {
            continue;
        }
        results.emplace_back(*ocr_opt);
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
    auto match_task_ptr = Task.get<MatchTaskInfo>(templ_task_name);
    m_roi = match_task_ptr->roi;
    MatchImageAnalyzerConfig::_set_task_info(*match_task_ptr);

    auto ocr_task_ptr = Task.get<OcrTaskInfo>(ocr_task_name);
    m_flag_rect_move = ocr_task_ptr->roi;
    OcrImageAnalyzerConfig::_set_task_info(*ocr_task_ptr);
}

void asst::OcrWithFlagTemplImageAnalyzer::set_flag_rect_move(Rect flag_rect_move)
{
    m_flag_rect_move = flag_rect_move;
}


void asst::OcrWithFlagTemplImageAnalyzer::sort_results_by_horizontal()
{
    if (!m_result) {
        return;
    }
    sort_by_horizontal_(m_result.value());
}

void asst::OcrWithFlagTemplImageAnalyzer::sort_results_by_vertical()
{
    if (!m_result) {
        return;
    }
    sort_by_vertical_(m_result.value());
}

void asst::OcrWithFlagTemplImageAnalyzer::sort_results_by_score()
{
    if (!m_result) {
        return;
    }
    sort_by_score_(m_result.value());
}

void asst::OcrWithFlagTemplImageAnalyzer::sort_results_by_required()
{
    if (!m_result) {
        return;
    }
    sort_by_required_(m_result.value(), OcrImageAnalyzerConfig::m_params.required);
}
