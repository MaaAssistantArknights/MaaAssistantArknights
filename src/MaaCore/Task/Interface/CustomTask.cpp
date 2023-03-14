#include "CustomTask.h"
#include "Task/ProcessTask.h"

asst::CustomTask::CustomTask(const AsstCallback& callback, Assistant* inst) : InterfaceTask(callback, inst, TaskType) {}

bool asst::CustomTask::set_params(const json::value& params)
{
    std::vector<std::string> tasks {};
    for (const auto& i : params.at("task_names").as_array()) {
        tasks.emplace_back(i.as_string());
    }
    auto custom_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    custom_task_ptr->set_tasks(tasks);
    m_subtasks.emplace_back(custom_task_ptr);
    return true;
}
