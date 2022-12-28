#include "SSSCopilotTask.h"

#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

asst::SSSCopilotTask::SSSCopilotTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_begin_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
      m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, inst, TaskType)),
      m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, inst, TaskType))
{
    m_begin_task_ptr->set_tasks({ inst_string() + "@SSSBegin" }).set_ignore_error(false);
    m_subtasks.emplace_back(m_begin_task_ptr);

    m_formation_task_ptr->set_data_resource(BattleFormationTask::DataResource::SSSCopilot);
    m_subtasks.emplace_back(m_formation_task_ptr);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_2_tp->set_tasks({ inst_string() + "@SSSTeamConfirm" }).set_ignore_error(false);
    m_subtasks.emplace_back(start_2_tp);

    m_subtasks.emplace_back(m_battle_task_ptr)->set_retry_times(0);
}

bool asst::SSSCopilotTask::set_params(const json::value& params)
{
    if (m_running) {
        Log.error("SSSCopilotTask not support set_params when running");
        return false;
    }

    auto filename_opt = params.find<std::string>("filename");
    if (!filename_opt) {
        Log.error("SSSCopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    if (!SSSCopilot.load(utils::path(*filename_opt))) {
        Log.error("SSSCopilotConfig parse failed");
        return false;
    }

    if (const auto& buff = SSSCopilot.get_data().buff; !buff.empty()) {
        Task.get<OcrTaskInfo>(inst_string() + "@SSSBuffChoose")->text = { buff };
    }

    bool with_formation = params.get("formation", false);
    m_formation_task_ptr->set_enable(with_formation);

    std::string support_unit_name = params.get("support_unit_name", std::string());
    m_formation_task_ptr->set_support_unit_name(std::move(support_unit_name));

    BattleFormationTask::AdditionalFormation additional_formation {
        .filter = BattleFormationTask::Filter::Cost,
        .double_click_filter = true,
        .role_counts = SSSCopilot.get_data().tool_men,
    };
    m_formation_task_ptr->append_additional_formation(std::move(additional_formation));

    return true;
}
