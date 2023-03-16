#include "OcrImageAnalyzer.h"

#include <regex>
#include <unordered_map>

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/Miscellaneous/OcrPack.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

const std::optional<asst::OcrImageAnalyzer::ResultVector>& asst::OcrImageAnalyzer::analyze() const
{
    m_result = std::nullopt;

    std::vector<ResultProc> preds_vec;

    preds_vec.emplace_back([](Result& ocr_res) -> bool {
        ocr_res.text = OcrConfig::get_instance().process_equivalence_class(ocr_res.text);
        return true;
    });

    if (!m_replace.empty()) {
        if (m_replace_full) {
            ResultProc text_replace = [&](Result& ocr_res) -> bool {
                for (const auto& [regex, new_str] : m_replace) {
                    if (std::regex_search(ocr_res.text, std::regex(regex))) {
                        ocr_res.text = new_str;
                    }
                }
                return true;
            };
            preds_vec.emplace_back(text_replace);
        }
        else {
            ResultProc text_replace = [&](Result& ocr_res) -> bool {
                for (const auto& [regex, new_str] : m_replace) {
                    ocr_res.text = std::regex_replace(ocr_res.text, std::regex(regex), new_str);
                }
                return true;
            };
            preds_vec.emplace_back(text_replace);
        }
    }

    if (!m_required.empty()) {
        if (m_full_match) {
            ResultProc required_match = [&](Result& ocr_res) -> bool {
                return ranges::find(m_required, ocr_res.text) != m_required.cend();
            };
            preds_vec.emplace_back(required_match);
        }
        else {
            ResultProc required_search = [&](Result& ocr_res) -> bool {
                auto is_sub = [&ocr_res](const std::string& str) -> bool {
                    if (ocr_res.text.find(str) == std::string::npos) {
                        return false;
                    }
                    ocr_res.text = str;
                    return true;
                };
                return ranges::find_if(m_required, is_sub) != m_required.cend();
            };
            preds_vec.emplace_back(required_search);
        }
    }

    preds_vec.emplace_back(m_pred);

    ResultProc all_pred = [&](Result& ocr_res) -> bool {
        for (const auto& pred : preds_vec) {
            if (pred && !pred(ocr_res)) {
                return false;
            }
        }
        return true;
    };

    OcrPack* ocr_ptr = nullptr;
    if (m_use_char_model) {
        ocr_ptr = &CharOcr::get_instance();
    }
    else {
        ocr_ptr = &WordOcr::get_instance();
    }
    auto results = ocr_ptr->recognize(m_image, m_roi, all_pred, m_without_det);
    ocr_ptr = nullptr;

    if (results.empty()) {
        return std::nullopt;
    }

    sort_(results, m_sorting);
    m_result = results;

    return m_result;
}

void asst::OcrImageAnalyzer::set_use_cache(bool is_use) noexcept
{
    m_use_cache = is_use;
}

void asst::OcrImageAnalyzer::set_required(std::vector<std::string> required) noexcept
{
    ranges::for_each(required,
                     [](std::string& str) { str = OcrConfig::get_instance().process_equivalence_class(str); });
    m_required = std::move(required);
}

void asst::OcrImageAnalyzer::set_replace(const std::unordered_map<std::string, std::string>& replace,
                                         bool replace_full) noexcept
{
    m_replace = {};
    for (auto&& [key, val] : replace) {
        auto new_key = OcrConfig::get_instance().process_equivalence_class(key);
        // do not create new_val as val is user-provided, and can avoid issues like 夕 and katakana タ
        m_replace.emplace(std::move(new_key), val);
    }
    m_replace_full = replace_full;
}

void asst::OcrImageAnalyzer::set_sorting(Sorting s)
{
    m_sorting = s;
}

void asst::OcrImageAnalyzer::set_task_info(OcrTaskInfo task_info) noexcept
{
    set_required(std::move(task_info.text));
    m_full_match = task_info.full_match;
    set_replace(task_info.replace_map, task_info.replace_full);
    m_use_cache = task_info.cache;
    m_use_char_model = task_info.is_ascii;

    if (m_use_cache && !m_region_of_appeared.empty()) {
        m_roi = m_region_of_appeared;
        m_without_det = true;
    }
    else {
        set_roi(task_info.roi);
        m_without_det = task_info.without_det;
    }
}

void asst::OcrImageAnalyzer::set_task_info(std::shared_ptr<TaskInfo> task_ptr)
{
    set_task_info(*std::dynamic_pointer_cast<OcrTaskInfo>(task_ptr));
}

void asst::OcrImageAnalyzer::set_task_info(const std::string& task_name)
{
    set_task_info(Task.get(task_name));
}

void asst::OcrImageAnalyzer::set_region_of_appeared(Rect region) noexcept
{
    m_region_of_appeared = region;
    if (m_use_cache && !m_region_of_appeared.empty()) {
        m_roi = m_region_of_appeared;
        m_without_det = true;
    }
}

void asst::OcrImageAnalyzer::set_use_char_model(bool enable) noexcept
{
    m_use_char_model = enable;
}

void asst::OcrImageAnalyzer::set_pred(const ResultProc& pred)
{
    m_pred = pred;
}

void asst::OcrImageAnalyzer::sort_(ResultVector& res, Sorting method) const
{
    switch (method) {
    case Sorting::None:
        break;
    case Sorting::ByScore:
        sort_by_score_(res);
        break;
    case Sorting::ByRequired:
        sort_by_required_(res);
        break;
    case Sorting::ByHorizontal:
        sort_by_horizontal_(res);
        break;
    case Sorting::ByVertical:
        sort_by_vertical_(res);
        break;
    }
}

void asst::OcrImageAnalyzer::sort_by_score_(ResultVector& res) const
{
    ranges::sort(res, std::greater {}, std::mem_fn(&Result::score));
}

void asst::OcrImageAnalyzer::sort_by_required_(ResultVector& res) const
{
    if (m_required.empty()) {
        return;
    }

    std::unordered_map<std::string, size_t> req_cache;
    for (size_t i = 0; i != m_required.size(); ++i) {
        req_cache.emplace(m_required.at(i), i + 1);
    }

    // 不在 m_required 中的将被排在最后
    ranges::sort(res, [&req_cache](const auto& lhs, const auto& rhs) -> bool {
        size_t lvalue = req_cache[lhs.text];
        size_t rvalue = req_cache[rhs.text];
        if (lvalue == 0) {
            return false;
        }
        else if (rvalue == 0) {
            return true;
        }
        return lvalue < rvalue;
    });
}

void asst::OcrImageAnalyzer::sort_by_horizontal_(ResultVector& res) const
{
    // +---+
    // |1 2|
    // |3 4|
    // +---+
    ranges::sort(res, [](const Result& lhs, const Result& rhs) -> bool {
        if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
}

void asst::OcrImageAnalyzer::sort_by_vertical_(ResultVector& res) const
{
    // +---+
    // |1 3|
    // |2 4|
    // +---+
    ranges::sort(res, [](const Result& lhs, const Result& rhs) -> bool {
        if (std::abs(lhs.rect.x - rhs.rect.x) < 5) { // x差距较小则理解为是同一排的，按y排序
            return lhs.rect.y < rhs.rect.y;
        }
        else {
            return lhs.rect.x < rhs.rect.x;
        }
    });
}
