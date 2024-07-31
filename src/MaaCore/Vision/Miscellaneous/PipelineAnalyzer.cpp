#include "PipelineAnalyzer.h"

#include <regex>
#include <utility>

#include "Config/TaskData.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

using namespace asst;

PipelineAnalyzer::ResultOpt PipelineAnalyzer::analyze() const
{
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

        // Log.trace(__FUNCTION__, task_ptr->name);
        switch (task_ptr->algorithm) {
        case AlgorithmType::JustReturn: {
            return Result { .task_ptr = task_ptr };
        } break;

        case AlgorithmType::MatchTemplate:
            if (auto match_opt = match(task_ptr)) {
                Log.trace(__FUNCTION__, "| MatchTemplate", task_ptr->name);
                return Result { .task_ptr = task_ptr, .result = *match_opt, .rect = match_opt->rect };
            }
            break;
        case AlgorithmType::OcrDetect:
            if (auto ocr_opt = ocr(task_ptr)) {
                Log.trace(__FUNCTION__, "| OcrDetect", task_ptr->name, *ocr_opt);
                return Result { .task_ptr = task_ptr, .result = ocr_opt->front(), .rect = ocr_opt->front().rect };
            }
            break;
        default:
            break;
        }
    }
    return std::nullopt;
}

Matcher::ResultOpt PipelineAnalyzer::match(const std::shared_ptr<TaskInfo>& task_ptr) const
{
    Matcher match_analyzer(m_image, m_roi);

    const auto match_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr);
    if (ranges::all_of(match_task_ptr->templ_thresholds, [](double t) { return t > 1.0; })) {
        Log.info(match_task_ptr->name, "'s threshold is", match_task_ptr->templ_thresholds, ", just skip");
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

OCRer::ResultsVecOpt PipelineAnalyzer::ocr(const std::shared_ptr<TaskInfo>& task_ptr) const
{
    const auto ocr_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(task_ptr);

    bool det = !ocr_task_ptr->without_det;
    bool use_cache = m_inst && ocr_task_ptr->cache;

    std::optional<Rect> cache_opt;
    if (use_cache) {
        cache_opt = status()->get_rect(ocr_task_ptr->name);
    }

    OCRer::ResultsVec result_vec;

    if (det) {
        OCRer analyzer(m_image, m_roi);
        analyzer.set_task_info(ocr_task_ptr);
        if (use_cache && cache_opt) {
            analyzer.set_roi(*cache_opt);
            analyzer.set_without_det(true);
        }
        auto result_opt = analyzer.analyze();
        if (!result_opt) {
            return std::nullopt;
        }
        result_vec = std::move(*result_opt);
    }
    else {
        RegionOCRer analyzer(m_image, m_roi);
        analyzer.set_task_info(ocr_task_ptr);
        if (use_cache && cache_opt) {
            analyzer.set_roi(*cache_opt);
        }
        auto result_opt = analyzer.analyze();
        if (!result_opt) {
            return std::nullopt;
        }
        result_vec = { std::move(*result_opt) };
    }

    if (use_cache && !result_vec.empty()) {
        status()->set_rect(ocr_task_ptr->name, result_vec.front().rect);
    }

    return result_vec;
}
