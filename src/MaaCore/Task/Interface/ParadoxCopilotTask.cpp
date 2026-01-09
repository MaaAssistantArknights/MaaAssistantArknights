#include "ParadoxCopilotTask.h"

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/ParadoxRecognitionTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::ParadoxCopilotTask::ParadoxCopilotTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_paradox_task_ptr(std::make_shared<ParadoxRecognitionTask>(callback, inst, TaskType)),
    m_start_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
    m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, inst, TaskType)),
    m_stop_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    m_paradox_task_ptr->set_battle_task_ptr(m_battle_task_ptr);
    m_battle_task_ptr->set_retry_times(0);
    m_stop_task_ptr->set_tasks({ "Copilot@ClickCornerUntilEndOfAction" });
    m_start_task_ptr->set_tasks({ "BattleStartAll" }).set_retry_times(3).set_ignore_error(false);

    /*
    m_subtasks.emplace_back(m_paradox_task_ptr);
    m_subtasks.emplace_back(m_start_task_ptr);
    m_subtasks.emplace_back(m_battle_task_ptr);
    m_subtasks.emplace_back(m_stop_task_ptr);
    */
}

bool asst::ParadoxCopilotTask::set_params(const json::value& params)
{
    LogTraceFunction;

    m_subtasks.clear();
    auto single_opt = params.find<std::string>("filename");
    if (single_opt) {
        std::string file_name;
        if (!Copilot.load(*single_opt)) {
            Log.error("CopilotConfig parse failed");
            return false;
        }
        file_name = utils::path_to_utf8_string(*single_opt);
        const auto& stage_name = Copilot.get_stage_name();
        if (!m_battle_task_ptr->set_stage_name(stage_name)) {
            Log.error("Not support stage");
            return false;
        }

        m_subtasks.emplace_back(m_start_task_ptr);
        m_subtasks.emplace_back(m_battle_task_ptr);
        m_subtasks.emplace_back(m_stop_task_ptr);
        return true;
    }

    auto batch_opt = params.find<std::vector<std::string>>("list");
    if (batch_opt) {
        m_subtasks.reserve(batch_opt->size() * 4);
        for (const auto& item : *batch_opt) {
            m_paradox_task_ptr->add_oper(item);
            m_subtasks.emplace_back(m_paradox_task_ptr);
            m_subtasks.emplace_back(m_start_task_ptr);
            m_subtasks.emplace_back(m_battle_task_ptr);
            m_subtasks.emplace_back(m_stop_task_ptr);
        }
        return true;
    }

    return false;
}
