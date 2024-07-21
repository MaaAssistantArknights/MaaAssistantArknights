#include "MatcherConfig.h"

#include "Config/TaskData.h"

using namespace asst;

void MatcherConfig::set_params(Params params)
{
    m_params = std::move(params);
}

void MatcherConfig::set_task_info(const std::shared_ptr<TaskInfo>& task_ptr)
{
    _set_task_info(*std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr));
}

void MatcherConfig::set_task_info(const std::string& task_name)
{
    set_task_info(Task.get(task_name));
}

void MatcherConfig::set_templ(std::variant<std::string, cv::Mat> templ)
{
    m_params.templs = { std::move(templ) };
}

// void asst::MatcherConfig::set_templ(std::vector<std::variant<std::string, cv::Mat>> templs)
//{
//     m_params.templs = std::move(templs);
// }

void MatcherConfig::set_threshold(double templ_thres) noexcept
{
    m_params.templ_thres = { templ_thres };
}

void asst::MatcherConfig::set_threshold(std::vector<double> templ_thres) noexcept
{
    m_params.templ_thres = std::move(templ_thres);
}

void MatcherConfig::set_mask_range(int lower, int upper, bool mask_with_src, bool mask_with_close)
{
    m_params.mask_range = std::make_pair(lower, upper);
    m_params.mask_with_src = mask_with_src;
    m_params.mask_with_close = mask_with_close;
}

void MatcherConfig::set_method(MatchMethod method) noexcept
{
    m_params.methods = { method };
}

void MatcherConfig::_set_task_info(MatchTaskInfo task_info)
{
    m_params.templs.clear();
    ranges::copy(task_info.templ_names, std::back_inserter(m_params.templs));
    m_params.templ_thres = std::move(task_info.templ_thresholds);
    m_params.mask_range = std::move(task_info.mask_range);
    m_params.methods = std::move(task_info.methods);

    _set_roi(task_info.roi);
}
