#include "ProcessTaskImageAnalyzer.h"

#include <regex>
#include <utility>

#include "Config/TaskData.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

asst::ProcessTaskImageAnalyzer::ProcessTaskImageAnalyzer(const cv::Mat& image, std::vector<std::string> tasks_name,
                                                         Assistant* inst)
    : AbstractImageAnalyzer(image, inst), m_ocr_analyzer(nullptr), m_ocr_with_preprocess_analyzer(nullptr),
      m_match_analyzer(nullptr), m_tasks_name(std::move(tasks_name))
{}

asst::ProcessTaskImageAnalyzer::~ProcessTaskImageAnalyzer() = default;

bool asst::ProcessTaskImageAnalyzer::match_analyze(const std::shared_ptr<TaskInfo>& task_ptr)
{
    if (!m_match_analyzer) {
        m_match_analyzer = std::make_unique<MatchImageAnalyzer>(m_image);
    }
    const auto match_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr);
    if (match_task_ptr->templ_threshold > 1.0) {
        Log.trace(match_task_ptr->name, "'s threshold is", match_task_ptr->templ_threshold, ", just skip");
        return false;
    }
    m_match_analyzer->set_region_of_appeared(Rect());
    m_match_analyzer->set_task_info(match_task_ptr);
    if (m_inst) {
        auto region_opt = status()->get_rect(match_task_ptr->name);
        if (region_opt) {
            m_match_analyzer->set_region_of_appeared(region_opt.value());
        }
    }

    if (m_match_analyzer->analyze()) {
        m_result = match_task_ptr;
        m_result_rect = m_match_analyzer->get_result().rect;
        if (m_inst) {
            status()->set_rect(match_task_ptr->name, m_result_rect);
        }
        return true;
    }
    return false;
}

bool asst::ProcessTaskImageAnalyzer::ocr_analyze(const std::shared_ptr<TaskInfo>& task_ptr)
{
    std::shared_ptr<OcrTaskInfo> ocr_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(task_ptr);

    // 先尝试从缓存的结果里找
    // for (const TextRect& tr : m_ocr_cache) {
    //    TextRect temp = tr;
    //    for (const auto& [regex, new_str] : ocr_task_ptr->replace_map) {
    //        temp.text = std::regex_replace(temp.text, std::regex(regex), new_str);
    //    }
    //    for (const auto& text : ocr_task_ptr->text) {
    //        bool flag = false;
    //        if (ocr_task_ptr->full_match) {
    //            if (temp.text == text) {
    //                flag = true;
    //            }
    //        }
    //        else if (temp.text.find(text) != std::string::npos) {
    //            flag = true;
    //        }
    //        // 即使找到了，也得在ROI里才行，不然过滤掉
    //        if (flag && ocr_task_ptr->roi.include(tr.rect)) {
    //            m_result = ocr_task_ptr;
    //            m_result_rect = tr.rect;
    //            ocr_task_ptr->region_of_appeared = m_result_rect;
    //            Log.trace("ProcessTaskImageAnalyzer::ocr_analyze | found in cache", tr.to_string());
    //            return true;
    //        }
    //    }
    //}
    std::unique_ptr<OcrImageAnalyzer>* analyzer_ptr = nullptr;
    if (ocr_task_ptr->without_det) {
        if (!m_ocr_with_preprocess_analyzer) {
            m_ocr_with_preprocess_analyzer = std::make_unique<OcrWithPreprocessImageAnalyzer>(m_image);
        }
        analyzer_ptr = decltype(analyzer_ptr)(&m_ocr_with_preprocess_analyzer);
    }
    else {
        if (!m_ocr_analyzer) {
            m_ocr_analyzer = std::make_unique<OcrImageAnalyzer>(m_image);
        }
        analyzer_ptr = &m_ocr_analyzer;
    }
    if (m_inst) {
        (*analyzer_ptr)->set_region_of_appeared(status()->get_rect(ocr_task_ptr->name).value_or(Rect()));
    }
    (*analyzer_ptr)->set_task_info(ocr_task_ptr);

    bool ret = (*analyzer_ptr)->analyze();

    if (ret) {
        (*analyzer_ptr)->sort_result_by_required();
        const auto& ocr_result = (*analyzer_ptr)->get_result();
        auto& res = ocr_result.front();
        m_result = ocr_task_ptr;
        m_result_rect = res.rect;
        if (m_inst) {
            status()->set_rect(ocr_task_ptr->name, m_result_rect);
        }
        // m_ocr_cache.insert(m_ocr_cache.end(), ocr_result.begin(), ocr_result.end());
        Log.trace(__FUNCTION__, "| found", res);
    }
    return ret;
}

void asst::ProcessTaskImageAnalyzer::reset() noexcept
{
    // m_ocr_cache.clear();
    m_ocr_analyzer = nullptr;
    m_match_analyzer = nullptr;
}

bool asst::ProcessTaskImageAnalyzer::analyze()
{
    m_result = nullptr;
    m_result_rect = Rect();

    for (const std::string& task_name : m_tasks_name) {
        auto task_ptr = Task.get(task_name);
        // 可能有配置错误，导致不存在对应的任务
        if (task_ptr == nullptr) {
            Log.error("Invalid task", task_name);
            continue;
        }

        switch (task_ptr->algorithm) {
        case AlgorithmType::JustReturn:
            m_result = task_ptr;
            return true;
        case AlgorithmType::MatchTemplate:
            if (match_analyze(task_ptr)) {
                return true;
            }
            break;
        case AlgorithmType::OcrDetect:
            if (ocr_analyze(task_ptr)) {
                return true;
            }
            break;
        default:
            break;
        }
    }
    return false;
}

void asst::ProcessTaskImageAnalyzer::set_image(const cv::Mat& image)
{
    AbstractImageAnalyzer::set_image(image);
    reset();
}

void asst::ProcessTaskImageAnalyzer::set_tasks(std::vector<std::string> tasks_name)
{
    m_tasks_name = std::move(tasks_name);
}
