#include "OcrImageAnalyzer.h"

#include <regex>
#include <unordered_map>

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/Miscellaneous/OcrPack.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

const asst::OcrImageAnalyzer::ResultsVecOpt& asst::OcrImageAnalyzer::analyze()
{
    m_result = std::nullopt;

    OcrPack* ocr_ptr = nullptr;
    if (m_use_char_model) {
        ocr_ptr = &CharOcr::get_instance();
    }
    else {
        ocr_ptr = &WordOcr::get_instance();
    }
    ResultsVec raw_results = ocr_ptr->recognize(make_roi(m_image, m_roi), m_without_det);
    ocr_ptr = nullptr;

    /* post process */
    ResultsVec results_vec;
    for (Result& res : raw_results) {
        if (res.text.empty() || res.score > 1.0) {
            continue;
        }

        postproc_rect_(res);
        postproc_trim_(res);
        postproc_equivalence_(res);
        postproc_replace_(res);

        if (!filter_and_replace_by_required_(res)) {
            continue;
        }

        results_vec.emplace_back(std::move(res));
    }

    if (results_vec.empty()) {
        return std::nullopt;
    }

    m_result = results_vec;
    return m_result;
}

void asst::OcrImageAnalyzer::sort_results_by_horizontal()
{
    if (!m_result) {
        return;
    }
    sort_by_horizontal_(m_result.value());
}

void asst::OcrImageAnalyzer::sort_results_by_vertical()
{
    if (!m_result) {
        return;
    }
    sort_by_vertical_(m_result.value());
}

void asst::OcrImageAnalyzer::sort_results_by_score()
{
    if (!m_result) {
        return;
    }
    sort_by_score_(m_result.value());
}

void asst::OcrImageAnalyzer::sort_results_by_required() {
    if (!m_result) {
        return;
    }
    sort_by_required_(m_result.value(), m_required);
}

void asst::OcrImageAnalyzer::set_required(std::vector<std::string> required) noexcept
{
    ranges::transform(required, required.begin(),
                      [](const std::string& str) { return OcrConfig::get_instance().process_equivalence_class(str); });
    m_required = std::move(required);
}

void asst::OcrImageAnalyzer::set_replace(const std::unordered_map<std::string, std::string>& replace,
                                         bool replace_full) noexcept
{
    m_replace.clear();
    m_replace.reserve(replace.size());

    for (auto&& [key, val] : replace) {
        auto new_key = OcrConfig::get_instance().process_equivalence_class(key);
        // do not create new_val as val is user-provided, and can avoid issues like 夕 and katakana タ
        m_replace.emplace(std::move(new_key), val);
    }
    m_replace_full = replace_full;
}

void asst::OcrImageAnalyzer::_set_task_info(OcrTaskInfo task_info) noexcept
{
    set_required(std::move(task_info.text));
    m_full_match = task_info.full_match;
    set_replace(task_info.replace_map, task_info.replace_full);
    m_use_char_model = task_info.is_ascii;
    m_without_det = task_info.without_det;
    set_roi(task_info.roi);
}

void asst::OcrImageAnalyzer::postproc_rect_(Result& res)
{
    if (m_without_det) {
        res.rect = m_roi;
    }
    else {
        res.rect.x += m_roi.x;
        res.rect.y += m_roi.y;
    }
}

void asst::OcrImageAnalyzer::postproc_trim_(Result& res)
{
    utils::string_trim_(res.text);
}

void asst::OcrImageAnalyzer::postproc_equivalence_(Result& res)
{
    auto& ocr_config = OcrConfig::get_instance();
    res.text = ocr_config.process_equivalence_class(res.text);
}

void asst::OcrImageAnalyzer::postproc_replace_(Result& res)
{
    if (m_replace.empty()) {
        return;
    }

    for (const auto& [regex, new_str] : m_replace) {
        if (m_replace_full) {
            if (std::regex_search(res.text, std::regex(regex))) {
                res.text = new_str;
            }
        }
        else {
            res.text = std::regex_replace(res.text, std::regex(regex), new_str);
        }
    }
}

bool asst::OcrImageAnalyzer::filter_and_replace_by_required_(Result& res)
{
    if (m_required.empty()) {
        return true;
    }

    if (m_full_match) {
        return ranges::find(m_required, res.text) != m_required.cend();
    }
    else {
        auto is_sub = [&res](const std::string& str) -> bool {
            if (res.text.find(str) == std::string::npos) {
                return false;
            }
            res.text = str;
            return true;
        };
        return ranges::find_if(m_required, is_sub) != m_required.cend();
    };
}

void asst::OcrImageAnalyzer::set_task_info(std::shared_ptr<TaskInfo> task_ptr)
{
    _set_task_info(*std::dynamic_pointer_cast<OcrTaskInfo>(task_ptr));
}

void asst::OcrImageAnalyzer::set_task_info(const std::string& task_name)
{
    set_task_info(Task.get(task_name));
}

void asst::OcrImageAnalyzer::set_use_char_model(bool enable) noexcept
{
    m_use_char_model = enable;
}

const asst::OcrImageAnalyzer::ResultsVecOpt& asst::OcrImageAnalyzer::result() const noexcept
{
    return m_result;
}
