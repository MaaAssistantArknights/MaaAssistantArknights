#include "OCRerConfig.h"

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/TaskData.h"

#include <string_view>

using namespace asst;

namespace
{
bool starts_with(std::string_view str, size_t pos, std::string_view prefix)
{
    return pos + prefix.size() <= str.size() && str.substr(pos, prefix.size()) == prefix;
}

bool is_regex_special(char ch)
{
    static constexpr std::string_view SpecialChars = R"(\.^$|()[]{}*+?)";
    return SpecialChars.find(ch) != std::string_view::npos;
}

std::string escape_regex_literal(std::string_view str)
{
    std::string result;
    result.reserve(str.size() * 2);
    for (char ch : str) {
        if (is_regex_special(ch)) {
            result += '\\';
        }
        result += ch;
    }
    return result;
}

std::string escape_char_class_literal(std::string_view str)
{
    std::string result;
    result.reserve(str.size() * 2);
    for (char ch : str) {
        if (ch == '\\' || ch == ']' || ch == '-' || ch == '^') {
            result += '\\';
        }
        result += ch;
    }
    return result;
}

std::string make_regex_alternation(const auto& eq_class)
{
    std::string result = "(?:";
    for (const auto& elem : eq_class) {
        result += escape_regex_literal(elem);
        result += '|';
    }
    result.back() = ')';
    return result;
}

std::string make_char_class_equivalence(const auto& eq_class)
{
    std::string result;
    for (const auto& elem : eq_class) {
        result += escape_char_class_literal(elem);
    }
    return result;
}

bool is_char_class_range_endpoint(std::string_view regex, size_t pos, size_t length)
{
    const bool is_range_start = pos + length < regex.size() && regex[pos + length] == '-';
    const bool is_range_end = pos > 0 && regex[pos - 1] == '-';
    return is_range_start || is_range_end;
}

std::string process_regex_equivalence_class(std::string_view regex, const auto& eq_classes)
{
    std::string result;
    result.reserve(regex.size());

    // OcrTaskInfo::replace_map keys are regex patterns, not plain text. A blind text replacement here can inject
    // regex syntax into the wrong context, e.g. "[Oo]" + ["o", "о"] used to become "[O(?:o|о)]", making ':' a
    // character-class member and replacing time strings like "03:53:4" with "0305304".
    bool in_char_class = false;
    bool escaped = false;

    for (size_t i = 0; i < regex.size();) {
        if (escaped) {
            result += regex[i++];
            escaped = false;
            continue;
        }

        const char ch = regex[i];
        if (ch == '\\') {
            result += ch;
            escaped = true;
            ++i;
            continue;
        }
        if (ch == '[') {
            in_char_class = true;
            result += ch;
            ++i;
            continue;
        }
        if (ch == ']' && in_char_class) {
            in_char_class = false;
            result += ch;
            ++i;
            continue;
        }

        bool replaced = false;
        for (const auto& eq_class : eq_classes) {
            if (eq_class.size() <= 1) {
                continue;
            }

            for (const auto& elem : eq_class) {
                if (elem.empty() || !starts_with(regex, i, elem)) {
                    continue;
                }

                if (in_char_class) {
                    // Inside [...] only literal class members are valid. Outside a class, use alternation.
                    if (is_char_class_range_endpoint(regex, i, elem.size())) {
                        break;
                    }
                    result += make_char_class_equivalence(eq_class);
                }
                else {
                    result += make_regex_alternation(eq_class);
                }
                i += elem.size();
                replaced = true;
                break;
            }
            if (replaced) {
                break;
            }
        }

        if (!replaced) {
            result += regex[i++];
        }
    }

    return result;
}
}

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

void OCRerConfig::set_replace(
    const std::vector<std::pair<std::string, std::string>>& replace,
    bool replace_full) noexcept
{
    m_params.replace.clear();
    m_params.replace.reserve(replace.size());

    auto& ocr_config = OcrConfig::get_instance();
    const auto eq_classes = ocr_config.get_eq_classes();
    for (auto&& [key, val] : replace) {
        std::string new_key = process_regex_equivalence_class(key, eq_classes);
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

    _set_roi(task_info.roi);
}
