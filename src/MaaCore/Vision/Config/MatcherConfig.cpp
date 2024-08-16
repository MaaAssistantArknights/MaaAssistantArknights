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

void MatcherConfig::set_mask_range(int lower, int upper, bool mask_src, bool mask_close)
{
    m_params.mask_ranges = { MatchTaskInfo::GrayRange { lower, upper } };
    m_params.mask_src = mask_src;
    m_params.mask_close = mask_close;
}

void MatcherConfig::set_mask_ranges(MatchTaskInfo::Ranges mask_ranges, bool mask_src, bool mask_close)
{
    m_params.mask_ranges = std::move(mask_ranges);
    m_params.mask_src = mask_src;
    m_params.mask_close = mask_close;
}

void MatcherConfig::set_color_scales(MatchTaskInfo::Ranges color_scales, bool color_close)
{
    m_params.color_scales = std::move(color_scales);
    m_params.color_close = color_close;
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
    m_params.mask_ranges = std::move(task_info.mask_ranges);
    m_params.color_scales = std::move(task_info.color_scales);
    m_params.color_close = task_info.color_close;
    m_params.methods = std::move(task_info.methods);

    _set_roi(task_info.roi);
}
