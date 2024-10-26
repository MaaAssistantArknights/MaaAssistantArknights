#include "CustomTask.h"
#include "Task/ProcessTask.h"

#include "Utils/Logger.hpp"

asst::CustomTask::CustomTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
    LogTraceFunction;
}

bool asst::CustomTask::set_params(const json::value& params)
{
    LogTraceFunction;

    auto tasks_opt = params.find<json::array>("task_names");
    if (!tasks_opt) {
        Log.error("set_params failed, task_names not found");
        return false;
    }
    std::vector<std::string> tasks;

    for (const auto& t : *tasks_opt) {
        if (!t.is_string()) {
            Log.error("set_params failed, task is not string");
            return false;
        }
        tasks.emplace_back(t.as_string());
    }
    auto custom_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    custom_task_ptr->set_tasks(std::move(tasks));
    m_subtasks.emplace_back(std::move(custom_task_ptr));
    return true;
}
