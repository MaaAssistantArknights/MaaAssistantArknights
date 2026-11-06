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

struct EquivalenceMatch
{
    const std::vector<std::string>* eq_class = nullptr;
    std::string_view elem;
};

static EquivalenceMatch
    find_equivalence_match(std::string_view text, size_t pos, const std::vector<std::vector<std::string>>& eq_classes)
{
    EquivalenceMatch match;
    for (const auto& eq_class : eq_classes) {
        if (eq_class.size() <= 1) {
            continue;
        }
        for (const auto& elem : eq_class) {
            if (elem.empty() || text.substr(pos, elem.size()) != std::string_view(elem.data(), elem.size())) {
                continue;
            }
            if (match.eq_class == nullptr || elem.size() > match.elem.size()) {
                match.eq_class = &eq_class;
                match.elem = std::string_view(elem.data(), elem.size());
            }
        }
    }
    return match;
}

static void append_class_equivalents(
    std::string& expanded,
    std::string_view literal,
    const std::vector<std::vector<std::string>>& eq_classes)
{
    for (const auto& eq_class : eq_classes) {
        if (eq_class.size() <= 1) {
            continue;
        }
        bool matched = false;
        for (const auto& elem : eq_class) {
            if (std::string_view(elem.data(), elem.size()) == literal) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            continue;
        }
        for (const auto& elem : eq_class) {
            if (std::string_view(elem.data(), elem.size()) != literal) {
                expanded += escape_class_literal(elem);
            }
        }
    }
}

static std::string
    expand_class_content(std::string_view content, const std::vector<std::vector<std::string>>& eq_classes)
{
    std::string expanded;
    expanded.reserve(content.size());

    for (size_t i = 0; i < content.size();) {
        if (content[i] == '\\') {
            expanded += content[i];
            if (i + 1 < content.size()) {
                expanded += content[i + 1];
            }
            i += 2;
            continue;
        }
        if (content[i] == '-') {
            expanded += '-';
            ++i;
            continue;
        }

        // 先把 a-z 当作一个整体复制，再把端点等价字符追加到区间之后，
        // 避免生成 [aа-z] 这种被 Boost 解释为倒序区间的结果。
        if (i + 2 < content.size() && content[i + 1] == '-' && content[i] != '-' && content[i + 2] != '-' &&
            static_cast<unsigned char>(content[i]) < 0x80 && static_cast<unsigned char>(content[i + 2]) < 0x80) {
            expanded.append(content.substr(i, 3));
            append_class_equivalents(expanded, content.substr(i, 1), eq_classes);
            append_class_equivalents(expanded, content.substr(i + 2, 1), eq_classes);
            i += 3;
            continue;
        }

        const EquivalenceMatch match = find_equivalence_match(content, i, eq_classes);
        if (match.eq_class == nullptr) {
            expanded += content[i++];
            continue;
        }

        expanded.append(content.substr(i, match.elem.size()));
        for (const auto& elem : *match.eq_class) {
            if (std::string_view(elem.data(), elem.size()) != match.elem) {
                expanded += escape_class_literal(elem);
            }
        }
        i += match.elem.size();
    }

    return expanded;
}

// 正则 key 需要语法感知地扩展等价字符：字面量位置生成 (?:a|b) 组，
// 字符类内追加等价字符；转义、分组前缀/名称、量词、锚点等保持原样。
static std::string
    expand_equivalence_classes_in_regex(std::string_view key, const std::vector<std::vector<std::string>>& eq_classes)
{
    std::string result;
    result.reserve(key.size());

    for (size_t i = 0; i < key.size();) {
        const char ch = key[i];

        if (ch == '\\') {
            if (i + 2 < key.size() && (key[i + 1] == 'k' || key[i + 1] == 'g') &&
                (key[i + 2] == '<' || key[i + 2] == '\'' || key[i + 2] == '{')) {
                const char open = key[i + 2];
                const char close = open == '<' ? '>' : (open == '\'' ? '\'' : '}');
                const size_t end = key.find(close, i + 3);
                if (end != std::string_view::npos) {
                    result.append(key.substr(i, end - i + 1));
                    i = end + 1;
                    continue;
                }
            }
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
            result += '[';
            if (!content.empty() && content.front() == '^') {
                result.append(content);
            }
            else {
                result.append(expand_class_content(content, eq_classes));
            }
            result += ']';
            i = close + 1;
            continue;
        }

        if (ch == '(') {
            if (i + 1 < key.size() && key[i + 1] == '?') {
                size_t prefix_len = 0;
                if (key.substr(i, 3) == "(?:" || key.substr(i, 3) == "(?=" || key.substr(i, 3) == "(?!" ||
                    key.substr(i, 3) == "(?>" || key.substr(i, 3) == "(?|") {
                    prefix_len = 3;
                }
                else if (key.substr(i, 4) == "(?<=" || key.substr(i, 4) == "(?<!") {
                    prefix_len = 4;
                }
                else if (key.substr(i, 4) == "(?P<" || key.substr(i, 3) == "(?<" || key.substr(i, 3) == "(?'") {
                    const char close = key[i + 2] == '\'' ? '\'' : '>';
                    const size_t end = key.find(close, i + 3);
                    if (end == std::string_view::npos) {
                        return std::string(key);
                    }
                    prefix_len = end - i + 1;
                }
                else if (key.substr(i, 4) == "(?P=" || key.substr(i, 3) == "(?#") {
                    const size_t end = key.find(')', i + 3);
                    if (end == std::string_view::npos) {
                        return std::string(key);
                    }
                    prefix_len = end - i + 1;
                }
                else {
                    size_t end = i + 2;
                    while (end < key.size() && ((key[end] >= 'A' && key[end] <= 'Z') ||
                                                (key[end] >= 'a' && key[end] <= 'z') || key[end] == '-')) {
                        ++end;
                    }
                    if (end > i + 2 && end < key.size() && (key[end] == ':' || key[end] == ')')) {
                        prefix_len = end - i + 1;
                    }
                    else {
                        // 未识别的 (?...) 构造：不重写，避免生成无效正则
                        return std::string(key);
                    }
                }

                result.append(key.substr(i, prefix_len));
                i += prefix_len;
                continue;
            }

            result += ch;
            ++i;
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

        const EquivalenceMatch match = find_equivalence_match(key, i, eq_classes);
        if (match.eq_class == nullptr) {
            result += ch;
            ++i;
            continue;
        }

        result += "(?:";
        for (size_t j = 0; j < match.eq_class->size(); ++j) {
            if (j != 0) {
                result += '|';
            }
            result += escape_regex_literal((*match.eq_class)[j]);
        }
        result += ')';
        i += match.elem.size();
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
