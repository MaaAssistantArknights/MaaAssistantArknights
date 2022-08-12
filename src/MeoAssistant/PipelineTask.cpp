#include "PipelineTask.h"

#include <meojson/json.hpp>

#include "ProcessTask.h"
#include "TaskData.h"

asst::PipelineTask::PipelineTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType))
{}

bool asst::PipelineTask::set_params(const json::value& params)
{
    if (m_running) {
        return false;
    }

    auto tasks_opt = params.find<json::array>("tasks");
    if (!tasks_opt) {
        return false;
    }

    std::vector<std::string> tasks_vec;
    for (const auto& task_json : tasks_opt.value()) {
        std::string task_name = task_json.as_string();
        if (!Task.get(task_name)) { // check task name exists
            return false;
        }
        tasks_vec.emplace_back(task_name);
    }
    if (tasks_vec.empty()) {
        return false;
    }
    m_task_ptr->set_tasks(tasks_vec);
    return true;
}
