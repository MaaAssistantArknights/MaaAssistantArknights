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
    ranges::transform(required, required.begin(),
                      [](const std::string& str) { return OcrConfig::get_instance().process_equivalence_class(str); });
    m_params.required = std::move(required);
}

void OCRerConfig::set_replace(const std::vector<std::pair<std::string, std::string>>& replace,
                              bool replace_full) noexcept
{
    m_params.replace.clear();
    m_params.replace.reserve(replace.size());

    for (auto&& [key, val] : replace) {
        auto new_key = OcrConfig::get_instance().process_equivalence_class(key);
        // do not create new_val as val is user-provided, and can avoid issues like 夕 and katakana タ
        m_params.replace.emplace_back(std::make_pair(std::move(new_key), val));
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

    _set_roi(task_info.roi);
}
