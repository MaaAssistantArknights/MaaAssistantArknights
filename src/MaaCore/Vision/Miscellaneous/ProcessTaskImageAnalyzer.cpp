#include "ProcessTaskImageAnalyzer.h"

#include <regex>
#include <utility>

#include "Config/TaskData.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

asst::MatchImageAnalyzer::ResultOpt asst::ProcessTaskImageAnalyzer::match_analyze(
    const std::shared_ptr<TaskInfo>& task_ptr)
{
    MatchImageAnalyzer match_analyzer(m_image, m_roi, m_inst);

    const auto match_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr);
    if (match_task_ptr->templ_threshold > 1.0) {
        Log.info(match_task_ptr->name, "'s threshold is", match_task_ptr->templ_threshold, ", just skip");
        return std::nullopt;
    }
    match_analyzer.set_task_info(match_task_ptr);

    bool use_cache = m_inst && match_task_ptr->cache;
    if (use_cache) {
        auto cache_opt = status()->get_rect(match_task_ptr->name);
        if (cache_opt) {
            match_analyzer.set_roi(*cache_opt);
        }
    }

    const auto& result_opt = match_analyzer.analyze();

    if (!result_opt) {
        return std::nullopt;
    }
    if (use_cache) {
        status()->set_rect(match_task_ptr->name, result_opt->rect);
    }

    return result_opt;
}

asst::OcrImageAnalyzer::ResultsVecOpt asst::ProcessTaskImageAnalyzer::ocr_analyze(
    const std::shared_ptr<TaskInfo>& task_ptr)
{
    const auto ocr_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(task_ptr);

    std::shared_ptr<OcrImageAnalyzer> ocr_analyzer_ptr = nullptr;
    if (ocr_task_ptr->without_det) {
        ocr_analyzer_ptr = std::make_shared<OcrWithPreprocessImageAnalyzer>(m_image, m_roi, m_inst);
    }
    else {
        ocr_analyzer_ptr = std::make_unique<OcrImageAnalyzer>(m_image, m_roi, m_inst);
    }
    ocr_analyzer_ptr->set_task_info(ocr_task_ptr);

    bool use_cache = m_inst && ocr_task_ptr->cache;
    if (use_cache) {
        auto cache_opt = status()->get_rect(ocr_task_ptr->name);
        if (cache_opt) {
            ocr_analyzer_ptr->set_roi(*cache_opt);
        }
    }

    const auto& result_opt = ocr_analyzer_ptr->analyze();
    if (!result_opt) {
        return std::nullopt;
    }

    ocr_analyzer_ptr->sort_results_by_required();

    if (use_cache) {
        status()->set_rect(ocr_task_ptr->name, result_opt->front().rect);
    }

    return result_opt;
}

const asst::ProcessTaskImageAnalyzer::ResultOpt& asst::ProcessTaskImageAnalyzer::analyze()
{
    m_result = std::nullopt;

    for (const std::string& task_name : m_tasks_name) {
        const auto& task_ptr = Task.get(task_name);
        // 可能有配置错误，导致不存在对应的任务
        if (task_ptr == nullptr) {
            Log.error("Invalid task", task_name);
#ifdef ASST_DEBUG
            throw std::runtime_error("Invalid task: " + task_name);
#endif
            continue;
        }

        switch (task_ptr->algorithm) {
        case AlgorithmType::JustReturn: {
            m_result = Result { .task_ptr = task_ptr };
            return m_result;
        } break;

        case AlgorithmType::MatchTemplate:
            if (auto match_opt = match_analyze(task_ptr)) {
                m_result = Result { .task_ptr = task_ptr, .result = *match_opt, .rect = match_opt->rect };
                return m_result;
            }
            break;
        case AlgorithmType::OcrDetect:
            if (auto ocr_opt = ocr_analyze(task_ptr)) {
                m_result = Result { .task_ptr = task_ptr, .result = ocr_opt->front(), .rect = ocr_opt->front().rect };
                return m_result;
            }
            break;
        default:
            break;
        }
    }
    return std::nullopt;
}
