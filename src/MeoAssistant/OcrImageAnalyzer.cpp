#include "OcrImageAnalyzer.h"

#include <regex>
#include <unordered_map>

#include "Logger.hpp"
#include "Resource.h"

bool asst::OcrImageAnalyzer::analyze()
{
    LogTraceFunction;

    m_ocr_result.clear();

    std::vector<TextRectProc> preds_vec;

    if (!m_replace.empty()) {
        TextRectProc text_replace = [&](TextRect& tr) -> bool {
            for (const auto& [regex, new_str] : m_replace) {
                tr.text = std::regex_replace(tr.text, std::regex(regex), new_str);
            }
            return true;
        };
        preds_vec.emplace_back(text_replace);
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

    if (m_roi.x < 0) {
        Log.warn("roi is out of range", m_roi.to_string());
        m_roi.x = 0;
    }
    if (m_roi.y < 0) {
        Log.warn("roi is out of range", m_roi.to_string());
        m_roi.y = 0;
    }
    if (m_roi.x + m_roi.width > m_image.cols) {
        Log.warn("roi is out of range", m_roi.to_string());
        m_roi.width = m_image.cols - m_roi.x;
    }
    if (m_roi.y + m_roi.height > m_image.rows) {
        Log.warn("roi is out of range", m_roi.to_string());
        m_roi.height = m_image.rows - m_roi.y;
    }

    m_ocr_result = Resrc.ocr().recognize(m_image, m_roi, all_pred, m_without_det);

    //log.trace("ocr result", m_ocr_result);
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
    m_required = std::move(required);
}

void asst::OcrImageAnalyzer::set_replace(std::unordered_map<std::string, std::string> replace) noexcept
{
    m_replace = std::move(replace);
}

void asst::OcrImageAnalyzer::set_task_info(OcrTaskInfo task_info) noexcept
{
    m_required = std::move(task_info.text);
    m_full_match = task_info.full_match;
    m_replace = std::move(task_info.replace_map);
    m_use_cache = task_info.cache;

    if (m_use_cache && !m_region_of_appeared.empty()) {
        m_roi = m_region_of_appeared;
        m_without_det = true;
    }
    else {
        set_roi(task_info.roi);
        m_without_det = false;
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
    ranges::sort(get_result(),
        [](const TextRect& lhs, const TextRect& rhs) -> bool {
            if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
                return lhs.rect.x < rhs.rect.x;
            }
            else {
                return lhs.rect.y < rhs.rect.y;
            }
        }
    );
}

void asst::OcrImageAnalyzer::sort_result_vertical()
{
    // 按位置排个序（顺序如下）
    // +---+
    // |1 3|
    // |2 4|
    // +---+
    ranges::sort(get_result(),
        [](const TextRect& lhs, const TextRect& rhs) -> bool {
            if (std::abs(lhs.rect.x - rhs.rect.x) < 5) { // x差距较小则理解为是同一排的，按y排序
                return lhs.rect.y < rhs.rect.y;
            }
            else {
                return lhs.rect.x < rhs.rect.x;
            }
        }
    );
}

void asst::OcrImageAnalyzer::sort_result_by_score()
{
    ranges::sort(get_result(),
        [](const TextRect& lhs, const TextRect& rhs) -> bool {
            return lhs.score > rhs.score;
        }
    );
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
    ranges::sort(result,
        [&req_cache](const auto& lhs, const auto& rhs) -> bool {
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
