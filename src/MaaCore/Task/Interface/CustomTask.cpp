#include "CustomTask.h"

#include "Config/TaskData.h"
#include "Task/Miscellaneous/ScreenshotTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::CustomTask::CustomTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_custom_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;
    m_custom_task_ptr->register_plugin<ScreenshotTaskPlugin>();
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
        if (Task.get(t.as_string()) == nullptr) {
            Log.error("set_params failed, task not found: ", t.as_string());
            return false;
        }
        tasks.emplace_back(t.as_string());
    }
    m_custom_task_ptr->set_tasks(std::move(tasks));
    m_subtasks.emplace_back(m_custom_task_ptr);
    return true;
}
