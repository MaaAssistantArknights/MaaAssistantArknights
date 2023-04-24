#include "TemplDetOCRer.h"

#include "Config/TaskData.h"
#include "MultiMatcher.h"
#include "RegionOCRer.h"

MAA_VISION_NS_BEGIN

TemplDetOCRer::ResultsVecOpt TemplDetOCRer::analyze() const
{
    MultiMatcher flag_analyzer(m_image, m_roi, m_inst);
    flag_analyzer.set_params(MatcherConfig::m_params);

    auto matched_vec_opt = flag_analyzer.analyze();
    if (!matched_vec_opt) {
        return std::nullopt;
    }
    auto& matched_vec = *matched_vec_opt;

    ResultsVec results;
    for (const auto& matched : matched_vec) {
        Rect roi = matched.rect.move(m_flag_rect_move);

        RegionOCRer ocr_analyzer(m_image, roi, m_inst);
        ocr_analyzer.set_params(OCRerConfig::m_params);
        auto ocr_opt = ocr_analyzer.analyze();
        if (!ocr_opt) {
            continue;
        }
        results.emplace_back(*ocr_opt);
    }

    if (results.empty()) {
        return std::nullopt;
    }

    return results;
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

MAA_VISION_NS_END
