#include "FeatureMatcherConfig.h"

#include "Config/TaskData.h"

void asst::FeatureMatcherConfig::set_task_info(const std::shared_ptr<TaskInfo>& task_ptr)
{
    _set_task_info(*std::dynamic_pointer_cast<FeatureMatchTaskInfo>(task_ptr));
}

void asst::FeatureMatcherConfig::set_task_info(const std::string& task_name)
{
    set_task_info(Task.get(task_name));
}

void asst::FeatureMatcherConfig::_set_task_info(FeatureMatchTaskInfo task_info)
{
    m_params.count = task_info.count;
    m_params.distance_ratio = task_info.ratio;
    m_params.detector = task_info.detector;
    m_params.templs = task_info.templ_names;
    _set_roi(task_info.roi);
    // m_params.green_mask = task_info.green_mask;
}
