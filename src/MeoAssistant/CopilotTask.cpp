#include "CopilotTask.h"

#include "Resource.h"
#include "BattleProcessTask.h"

asst::CopilotTask::CopilotTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, callback_arg, TaskType))
{
    m_subtasks.emplace_back(m_battle_task_ptr);
}

bool asst::CopilotTask::set_params(const json::value& params)
{
    if (!params.contains("filename") || !params.at("filename").is_string()
        || !params.contains("stage_name") || !params.at("stage_name").is_string()) {
        return false;
    }
    std::string filename = params.at("filename").as_string();
    std::string stage_name = params.at("stage_name").as_string();

    m_battle_task_ptr->set_stage_name(stage_name);

    return Resrc.battle().load(filename);
}
