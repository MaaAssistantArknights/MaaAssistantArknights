#include "OcrImageAnalyzerConfig.h"

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/TaskData.h"

void asst::OcrImageAnalyzerConfig::set_params(Params params)
{
    m_params = std::move(params);
}

void asst::OcrImageAnalyzerConfig::set_required(std::vector<std::string> required) noexcept
{
    ranges::transform(required, required.begin(),
                      [](const std::string& str) { return OcrConfig::get_instance().process_equivalence_class(str); });
    m_params.required = std::move(required);
}

void asst::OcrImageAnalyzerConfig::set_replace(const std::unordered_map<std::string, std::string>& replace,
                                               bool replace_full) noexcept
{
    m_params.replace.clear();
    m_params.replace.reserve(replace.size());

    for (auto&& [key, val] : replace) {
        auto new_key = OcrConfig::get_instance().process_equivalence_class(key);
        // do not create new_val as val is user-provided, and can avoid issues like 夕 and katakana タ
        m_params.replace.emplace(std::move(new_key), val);
    }
    m_params.replace_full = replace_full;
}

void asst::OcrImageAnalyzerConfig::set_task_info(std::shared_ptr<TaskInfo> task_ptr)
{
    _set_task_info(*std::dynamic_pointer_cast<OcrTaskInfo>(task_ptr));
}

void asst::OcrImageAnalyzerConfig::set_task_info(const std::string& task_name)
{
    set_task_info(Task.get(task_name));
}

void asst::OcrImageAnalyzerConfig::set_use_char_model(bool enable) noexcept
{
    m_params.use_char_model = enable;
}

void asst::OcrImageAnalyzerConfig::_set_task_info(OcrTaskInfo task_info)
{
    set_required(std::move(task_info.text));
    m_params.full_match = task_info.full_match;
    set_replace(task_info.replace_map, task_info.replace_full);
    m_params.use_char_model = task_info.is_ascii;
    m_params.without_det = task_info.without_det;

    _set_roi(task_info.roi);
}
