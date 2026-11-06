#include "OCRerConfig.h"

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/TaskData.h"

using namespace asst;

void OCRerConfig::set_params(Params params)
{
    m_params = std::move(params);
}

void OCRerConfig::set_required(std::vector<std::string> required) noexcept
{
    m_params.required.clear();
    m_params.required.reserve(required.size());

    auto& ocr_config = OcrConfig::get_instance();
    for (auto& str : required) {
        std::string equ_str = ocr_config.process_equivalence_class(str);
        m_params.required.emplace_back(std::move(str), std::move(equ_str));
    }
}

static std::string escape_regex_literal(std::string_view value)
{
    std::string escaped;
    for (const char ch : value) {
        if (std::string_view("\\^$.|?*+()[]{}").find(ch) != std::string_view::npos) {
            escaped += '\\';
        }
        escaped += ch;
    }
    return escaped;
}

static std::string escape_class_literal(std::string_view value)
{
    std::string escaped;
    for (const char ch : value) {
        if (ch == '\\' || ch == ']' || ch == '-' || ch == '^') {
            escaped += '\\';
        }
        escaped += ch;
    }
    return escaped;
}

// 正则 key 需要语法感知地扩展等价字符：字面量位置生成 (?:a|b) 组，
// 简单字符类内追加等价字符（[Oo] -> [Ooо]）；转义、分组、量词、锚点等保持原样。
static std::string expand_equivalence_classes_in_regex(
    std::string_view key,
    const std::vector<std::vector<std::string>>& eq_classes)
{
    std::string result;
    result.reserve(key.size());

    for (size_t i = 0; i < key.size();) {
        const char ch = key[i];

        if (ch == '\\') {
            result += ch;
            if (i + 1 < key.size()) {
                result += key[i + 1];
            }
            i += 2;
            continue;
        }

        if (ch == '[') {
            size_t close = i + 1;
            while (close < key.size()) {
                if (key[close] == '\\') {
                    close += 2;
                    continue;
                }
                if (key[close] == ']') {
                    break;
                }
                ++close;
            }
            if (close >= key.size()) {
                result.append(key.substr(i));
                break;
            }

            const std::string_view content = key.substr(i + 1, close - i - 1);
            const bool simple =
                !content.empty() && content.front() != '^' && content.find_first_of("\\-") == std::string_view::npos;
            if (!simple) {
                result.append(key.substr(i, close - i + 1));
                i = close + 1;
                continue;
            }

            std::string expanded(content);
            for (const auto& eq_class : eq_classes) {
                if (eq_class.size() <= 1) {
                    continue;
                }
                bool matched = false;
                for (const auto& elem : eq_class) {
                    if (content.find(elem) != std::string_view::npos) {
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    continue;
                }
                for (const auto& elem : eq_class) {
                    if (expanded.find(elem) == std::string::npos) {
                        expanded += escape_class_literal(elem);
                    }
                }
            }

            result += '[';
            result += expanded;
            result += ']';
            i = close + 1;
            continue;
        }

        if (ch == '{') {
            const size_t close = key.find('}', i + 1);
            if (close != std::string_view::npos) {
                result.append(key.substr(i, close - i + 1));
                i = close + 1;
                continue;
            }
        }

        if (std::string_view("(){}\\[].*+?|^$").find(ch) != std::string_view::npos) {
            result += ch;
            ++i;
            continue;
        }

        const std::vector<std::string>* matched_class = nullptr;
        std::string_view matched_elem;
        for (const auto& eq_class : eq_classes) {
            if (eq_class.size() <= 1) {
                continue;
            }
            for (const auto& elem : eq_class) {
                if (elem.empty() || key.compare(i, elem.size(), elem) != 0) {
                    continue;
                }
                if (matched_elem.empty() || elem.size() > matched_elem.size()) {
                    matched_class = &eq_class;
                    matched_elem = elem;
                }
            }
        }

        if (matched_class == nullptr) {
            result += ch;
            ++i;
            continue;
        }

        result += "(?:";
        for (size_t j = 0; j < matched_class->size(); ++j) {
            if (j != 0) {
                result += '|';
            }
            result += escape_regex_literal((*matched_class)[j]);
        }
        result += ')';
        i += matched_elem.size();
    }

    return result;
}

void OCRerConfig::set_replace(
    const std::vector<std::pair<std::string, std::string>>& replace,
    bool replace_full) noexcept
{
    m_params.replace.clear();
    m_params.replace.reserve(replace.size());

    for (auto&& [key, val] : replace) {
        auto& ocr_config = OcrConfig::get_instance();
        std::string new_key = key;
        if (key.find_first_of("\\[](){}.*+?|^$") == std::string::npos) {
            // 纯文本 key：保留原来的等价字符组注入
            for (const auto& eq_class : ocr_config.get_eq_classes()) {
                if (eq_class.size() <= 1) {
                    continue;
                }

                // eq_class: [s, S] -> regex: "(?:s|S)"
                std::string eq_classes_regex = "(?:";
                for (const auto& elem : eq_class) {
                    (eq_classes_regex += elem) += '|';
                }
                eq_classes_regex.pop_back();
                eq_classes_regex += ')';
                std::ranges::for_each(eq_class, [&](std::string_view elem) {
                    utils::string_replace_all_in_place(new_key, elem, eq_classes_regex);
                });
            }
        }
        else {
            new_key = expand_equivalence_classes_in_regex(key, ocr_config.get_eq_classes());
        }
        // do not create new_val as val is user-provided, and can avoid issues like 夕 and katakana タ
        m_params.replace.emplace_back(std::move(new_key), val);
    }
    m_params.replace_full = replace_full;
}

void OCRerConfig::set_task_info(std::shared_ptr<TaskInfo> task_ptr)
{
    _set_task_info(*std::dynamic_pointer_cast<OcrTaskInfo>(task_ptr));
}

void OCRerConfig::set_task_info(const std::string& task_name)
{
    set_task_info(Task.get(task_name));
}

void asst::OCRerConfig::set_without_det(bool without_det) noexcept
{
    m_params.without_det = without_det;
}

void OCRerConfig::set_use_char_model(bool enable) noexcept
{
    m_params.use_char_model = enable;
}

void asst::OCRerConfig::set_use_raw(bool value) noexcept
{
    m_params.use_raw = value;
}

void OCRerConfig::set_bin_threshold(int lower, int upper)
{
    m_params.bin_threshold_lower = lower;
    m_params.bin_threshold_upper = upper;
}

void OCRerConfig::set_bin_expansion(int expansion)
{
    m_params.bin_expansion = expansion;
}

void asst::OCRerConfig::set_bin_trim_threshold(int left, int right)
{
    m_params.bin_left_trim_threshold = left;
    m_params.bin_right_trim_threshold = right;
}

void OCRerConfig::_set_task_info(OcrTaskInfo task_info)
{
    set_required(std::move(task_info.text));
    m_params.full_match = task_info.full_match;
    set_replace(task_info.replace_map, task_info.replace_full);
    m_params.use_char_model = task_info.is_ascii;
    m_params.without_det = task_info.without_det;
    m_params.bin_threshold_lower = task_info.bin_threshold[0];
    m_params.bin_threshold_upper = task_info.bin_threshold[1];
    m_params.use_raw = task_info.use_raw;
    m_params.order_by = task_info.order_by;

    _set_roi(task_info.roi);
}
