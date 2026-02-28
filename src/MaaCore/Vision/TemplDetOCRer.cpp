#include "TemplDetOCRer.h"

#include "Config/TaskData.h"
#include "MultiMatcher.h"
#include "RegionOCRer.h"

using namespace asst;

TemplDetOCRer::ResultsVecOpt TemplDetOCRer::analyze() const
{
    MultiMatcher flag_analyzer(m_image, m_roi);
    flag_analyzer.set_params(MatcherConfig::m_params);

    auto matched_vec_opt = flag_analyzer.analyze();
    if (!matched_vec_opt) {
        return std::nullopt;
    }
    auto& matched_vec = *matched_vec_opt;

    ResultsVec results;
    for (const auto& matched : matched_vec) {
        Rect roi = matched.rect.move(m_flag_rect_move);

        RegionOCRer ocr_analyzer(m_image, roi);
        ocr_analyzer.set_params(OCRerConfig::m_params);
        auto ocr_opt = ocr_analyzer.analyze();
        if (!ocr_opt) {
            continue;
        }
        Result result;
        result.text = ocr_opt->text;
        result.abs_rect = ocr_opt->abs_rect;
        result.rect = ocr_opt->rect;
        result.score = ocr_opt->score;
        result.flag_rect = matched.rect;
        result.flag_score = matched.score;

        results.emplace_back(std::move(result));
    }

    if (results.empty()) {
        return std::nullopt;
    }

    m_result = std::move(results);
    return m_result;
}

void TemplDetOCRer::set_task_info(const std::string& templ_task_name, const std::string& ocr_task_name)
{
    auto match_task_ptr = Task.get<MatchTaskInfo>(templ_task_name);
    m_roi = match_task_ptr->roi;
    MatcherConfig::_set_task_info(*match_task_ptr);

    auto ocr_task_ptr = Task.get<OcrTaskInfo>(ocr_task_name);
    m_flag_rect_move = ocr_task_ptr->roi;
    OCRerConfig::_set_task_info(*ocr_task_ptr);
}

void TemplDetOCRer::set_flag_rect_move(Rect flag_rect_move)
{
    m_flag_rect_move = flag_rect_move;
}
