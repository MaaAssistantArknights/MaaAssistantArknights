#include "OcrImageAnalyzer.h"

#include <regex>
#include <unordered_map>

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/Miscellaneous/OcrPack.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

bool asst::OcrImageAnalyzer::analyze()
{
    // LogTraceFunction;

    m_ocr_result.clear();

    std::vector<TextRectProc> preds_vec;

    preds_vec.emplace_back([](TextRect& tr) -> bool {
        tr.text = OcrConfig::get_instance().process_equivalence_class(tr.text);
        return true;
    });

    if (!m_replace.empty()) {
        if (m_replace_full) {
            TextRectProc text_replace = [&](TextRect& tr) -> bool {
                for (const auto& [regex, new_str] : m_replace) {
                    if (std::regex_search(tr.text, std::regex(regex))) {
                        tr.text = new_str;
                    }
                }
                return true;
            };
            preds_vec.emplace_back(text_replace);
        }
        else {
            TextRectProc text_replace = [&](TextRect& tr) -> bool {
                for (const auto& [regex, new_str] : m_replace) {
                    tr.text = std::regex_replace(tr.text, std::regex(regex), new_str);
                }
                return true;
            };
            preds_vec.emplace_back(text_replace);
        }
    }

    if (!m_required.empty()) {
        if (m_full_match) {
            TextRectProc required_match = [&](TextRect& tr) -> bool {
                return ranges::find(m_required, tr.text) != m_required.cend();
            };
            preds_vec.emplace_back(required_match);
        }
        else {
            TextRectProc required_search = [&](TextRect& tr) -> bool {
                auto is_sub = [&tr](const std::string& str) -> bool {
                    if (tr.text.find(str) == std::string::npos) {
                        return false;
                    }
                    tr.text = str;
                    return true;
                };
                return ranges::find_if(m_required, is_sub) != m_required.cend();
            };
            preds_vec.emplace_back(required_search);
        }
    }

    preds_vec.emplace_back(m_pred);

    TextRectProc all_pred = [&](TextRect& tr) -> bool {
        for (const auto& pred : preds_vec) {
            if (pred && !pred(tr)) {
                return false;
            }
        }
        return true;
    };

    m_roi = correct_rect(m_roi, m_image);

    OcrPack* ocr_ptr = nullptr;
    if (m_use_char_model) {
        ocr_ptr = &CharOcr::get_instance();
    }
    else {
        ocr_ptr = &WordOcr::get_instance();
    }
    m_ocr_result = ocr_ptr->recognize(m_image, m_roi, all_pred, m_without_det);
    ocr_ptr = nullptr;

    // log.trace("ocr result", m_ocr_result);
    return !m_ocr_result.empty();
}

void asst::OcrImageAnalyzer::filter(const TextRectProc& filter_func)
{
    std::vector<TextRect> temp_result;

    for (auto&& tr : get_result()) {
        if (filter_func(tr)) {
            temp_result.emplace_back(std::move(tr));
        }
    }
    get_result() = std::move(temp_result);
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

std::vector<asst::TextRect>& asst::OcrImageAnalyzer::get_result() noexcept
{
    return m_ocr_result;
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

void asst::OcrImageAnalyzer::set_pred(const TextRectProc& pred)
{
    m_pred = pred;
}

const std::vector<asst::TextRect>& asst::OcrImageAnalyzer::get_result() const noexcept
{
    return m_ocr_result;
}

void asst::OcrImageAnalyzer::sort_result_horizontal()
{
    // 按位置排个序
    ranges::sort(get_result(), [](const TextRect& lhs, const TextRect& rhs) -> bool {
        if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
}

void asst::OcrImageAnalyzer::sort_result_vertical()
{
    // 按位置排个序（顺序如下）
    // +---+
    // |1 3|
    // |2 4|
    // +---+
    ranges::sort(get_result(), [](const TextRect& lhs, const TextRect& rhs) -> bool {
        if (std::abs(lhs.rect.x - rhs.rect.x) < 5) { // x差距较小则理解为是同一排的，按y排序
            return lhs.rect.y < rhs.rect.y;
        }
        else {
            return lhs.rect.x < rhs.rect.x;
        }
    });
}

void asst::OcrImageAnalyzer::sort_result_by_score()
{
    ranges::sort(get_result(), std::greater {}, std::mem_fn(&TextRect::score));
}

void asst::OcrImageAnalyzer::sort_result_by_required()
{
    if (m_required.empty()) {
        return;
    }

    std::unordered_map<std::string, size_t> req_cache;
    for (size_t i = 0; i != m_required.size(); ++i) {
        req_cache.emplace(m_required.at(i), i + 1);
    }

    auto& result = get_result();
    // 不在 m_required 中的将被排在最后
    ranges::sort(result, [&req_cache](const auto& lhs, const auto& rhs) -> bool {
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
